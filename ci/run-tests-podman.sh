#!/usr/bin/env bash
#
# Build fping and run its test suite inside an isolated Podman (or Docker)
# container without requiring root privileges on the host or leaving build
# artifacts in the working copy.

set -euo pipefail
unset CDPATH

CONTAINER_ENGINE="${CONTAINER_ENGINE:-podman}"
IMAGE_NAME="${FPING_TEST_IMAGE:-fping-test-env:ubuntu-24.04}"
REPO_ROOT="$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"

REBUILD_IMAGE=0
REL_TESTS=()

usage() {
    cat <<'EOF'
Usage: ci/run-tests-podman.sh [OPTIONS] [TEST_FILE...]

Run the fping test suite in an isolated container (uses podman by default;
set CONTAINER_ENGINE=docker to use Docker instead).

Options:
  --rebuild-image  Rebuild the cached test environment container image
  -h, --help       Show this help message

Examples:
  ci/run-tests-podman.sh
  ci/run-tests-podman.sh ci/test-16-json-output.pl
  ci/run-tests-podman.sh ci/unsafe-test-oiface.sh
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --rebuild-image)
            REBUILD_IMAGE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            while [ $# -gt 0 ]; do
                REL_TESTS+=("$1")
                shift
            done
            ;;
        -*)
            echo "Error: unknown option '$1' (try --help)" >&2
            exit 2
            ;;
        *)
            REL_TESTS+=("$1")
            shift
            ;;
    esac
done

NORMALIZED_TESTS=()
for arg in ${REL_TESTS[@]+"${REL_TESTS[@]}"}; do
    if [ -f "$arg" ]; then
        candidate="$arg"
    elif [ -f "$REPO_ROOT/${arg#./}" ]; then
        candidate="$REPO_ROOT/${arg#./}"
    else
        echo "Error: test file not found: '$arg'" >&2
        exit 2
    fi

    abs="$(cd -P -- "$(dirname -- "$candidate")" && pwd -P)/$(basename -- "$candidate")"
    case "$abs" in
        "$REPO_ROOT/"*)
            rel="${abs#"$REPO_ROOT/"}"
            ;;
        *)
            echo "Error: test file '$arg' is outside repository root '$REPO_ROOT'" >&2
            exit 2
            ;;
    esac

    case "$rel" in
        *.pl|*.sh)
            NORMALIZED_TESTS+=("$rel")
            ;;
        *)
            echo "Error: unsupported test file '$rel' (expected *.pl or *.sh)" >&2
            exit 2
            ;;
    esac
done

if ! command -v "$CONTAINER_ENGINE" >/dev/null 2>&1; then
    echo "Error: '$CONTAINER_ENGINE' not found in PATH" >&2
    exit 1
fi

if [ "$REBUILD_IMAGE" -eq 1 ] || ! "$CONTAINER_ENGINE" image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "==> Building cached test environment image ($IMAGE_NAME)..."
    "$CONTAINER_ENGINE" build -t "$IMAGE_NAME" - <<'DOCKERFILE_EOF'
FROM docker.io/library/ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq && \
    apt-get install -y -qq --no-install-recommends \
        autoconf \
        automake \
        build-essential \
        ca-certificates \
        iproute2 \
        libcap2-bin \
        libtest-command-perl \
        net-tools \
        netbase \
        perl \
        procps \
        sudo \
        tcpdump \
        tini && \
    rm -rf /var/lib/apt/lists/* && \
    (id -u runner >/dev/null 2>&1 || useradd -m -s /bin/bash runner) && \
    echo "runner ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/runner && \
    chmod 0440 /etc/sudoers.d/runner
ENTRYPOINT ["/usr/bin/tini", "-g", "--"]
DOCKERFILE_EOF
fi

IFS= read -r -d '' CONTAINER_SCRIPT <<'CONTAINER_EOF' || true
set -euo pipefail

export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export LC_ALL=C.UTF-8

# Prevent host search domains or NXDOMAIN wildcards from hijacking invalid test
# hostnames, unless internet-hosts testing is explicitly requested.
PRESERVE_DNS=0
for f in "$@"; do
    case "$f" in
        *test-14-ping-internet-hosts.pl) PRESERVE_DNS=1 ;;
    esac
done
if [ "$PRESERVE_DNS" -eq 0 ]; then
    host_name="$(hostname 2>/dev/null || true)"
    : > /etc/resolv.conf
    {
        printf '127.0.0.1 localhost localhost.localdomain%s\n' "${host_name:+ $host_name}"
        printf '::1 localhost ip6-localhost ip6-loopback%s\n' "${host_name:+ $host_name}"
    } > /etc/hosts
fi

sysctl -qw net.ipv6.conf.all.disable_ipv6=0 2>/dev/null || true
sysctl -qw net.ipv6.conf.default.disable_ipv6=0 2>/dev/null || true
sysctl -qw net.ipv6.conf.lo.disable_ipv6=0 2>/dev/null || true

sysctl -qw net.ipv4.ping_group_range="0 2147483647" 2>/dev/null || \
    sysctl -qw net.ipv4.ping_group_range="0 65534" 2>/dev/null || \
    sysctl -qw net.ipv4.ping_group_range="0 $(id -g runner)" 2>/dev/null || true

# In rootless Podman user namespaces, /sys carries MNT_LOCK_NOSUID|NODEV|NOEXEC,
# whereas iproute2's `ip netns exec` mounts sysfs without those flags (EPERM).
if /usr/sbin/ip netns add _fping_probe_ns 2>/dev/null; then
    if ! /usr/sbin/ip netns exec _fping_probe_ns true 2>/dev/null; then
        cat > /usr/local/sbin/ip <<'SHIM_EOF'
#!/bin/sh
if [ "${1:-}" = "netns" ] && [ "${2:-}" = "exec" ] && [ $# -ge 4 ]; then
    ns="$3"
    shift 3
    exec nsenter --net="/var/run/netns/$ns" -- unshare -m -- sh -eu -c '
        mount -t sysfs -o nosuid,nodev,noexec sysfs /sys
        exec "$@"
    ' sh "$@"
fi
exec /usr/sbin/ip "$@"
SHIM_EOF
        chmod 0755 /usr/local/sbin/ip
        hash -r
    fi
    /usr/sbin/ip netns del _fping_probe_ns 2>/dev/null || true
fi

mkdir -p /tmp/fping
tar -C /src \
    --exclude='./.git' \
    --exclude='./.jj' \
    --exclude='./.local' \
    --exclude='./autom4te.cache' \
    --exclude='./ci/tmp*' \
    -cf - . | tar -C /tmp/fping --no-same-owner -xf -
chown -R runner:runner /tmp/fping

echo "==> Building fping..."
sudo -u runner -- bash -euo pipefail -c '
    cd /tmp/fping
    if [ -f Makefile ]; then
        make distclean >/dev/null 2>&1 || true
    fi
    rm -f src/*.o src/fping src/fping6 src/*_test src/*.log src/*.trs src/*.gcda src/*.gcno
    autoreconf -i
    ./configure --enable-ipv4 --enable-ipv6 --enable-safe-limits --prefix=/opt/fping >/dev/null
    make -j"$(nproc)" >/dev/null
'

if [ $# -eq 0 ]; then
    echo "==> Running unit tests (make check)..."
    sudo -u runner -- bash -euo pipefail -c '
        cd /tmp/fping
        exec make check VERBOSE=1
    '
fi

chown root:root /tmp/fping/src/fping
chmod u+s /tmp/fping/src/fping

RUN_ALL=0
PL_TESTS=()
UNSAFE_PL_TESTS=()
SH_TESTS=()
if [ $# -eq 0 ]; then
    RUN_ALL=1
    while IFS= read -r f; do
        PL_TESTS+=("$f")
    done < <(cd /tmp/fping && ls ci/test-*.pl | grep -v internet-hosts)
    for f in /tmp/fping/ci/unsafe-test-*.pl; do
        if [ -f "$f" ]; then
            UNSAFE_PL_TESTS+=("ci/${f##*/}")
        fi
    done
    for f in /tmp/fping/ci/unsafe-test-*.sh; do
        if [ -f "$f" ]; then
            SH_TESTS+=("ci/${f##*/}")
        fi
    done
    for f in ci/test-oiface.sh ci/test-oiface-ipv6.sh; do
        if [ -f "/tmp/fping/$f" ]; then
            SH_TESTS+=("$f")
        fi
    done
else
    for f in "$@"; do
        case "$f" in
            ci/unsafe-test-*.pl|*/unsafe-test-*.pl|unsafe-test-*.pl) UNSAFE_PL_TESTS+=("$f") ;;
            *.pl) PL_TESTS+=("$f") ;;
            *.sh) SH_TESTS+=("$f") ;;
        esac
    done
fi

if [ "${#PL_TESTS[@]}" -gt 0 ]; then
    echo "==> Running prove (${#PL_TESTS[@]} test file(s))..."
    sudo -u runner -- bash -euo pipefail -c '
        cd /tmp/fping
        export PATH="/tmp/fping/src:$PATH"
        exec prove "$@"
    ' _ "${PL_TESTS[@]}"
fi

if [ "${#UNSAFE_PL_TESTS[@]}" -gt 0 ]; then
    echo "==> Running unsafe prove (${#UNSAFE_PL_TESTS[@]} test file(s))..."
    sudo -u runner -- bash -euo pipefail -c '
        cd /tmp/fping
        exec sudo env "PATH=/tmp/fping/src:$PATH" FPING_RUN_UNSAFE_TESTS=1 prove "$@"
    ' _ "${UNSAFE_PL_TESTS[@]}"
fi

run_sh_test() {
    sudo -u runner -- bash -euo pipefail -c '
        cd /tmp/fping
        sudo env "PATH=/tmp/fping/src:$PATH" FPING_RUN_UNSAFE_TESTS=1 bash "$1"
    ' _ "$1"
}

for sh_test in ${SH_TESTS[@]+"${SH_TESTS[@]}"}; do
    echo "==> Running $sh_test..."
    if [ "$RUN_ALL" -eq 1 ]; then
        log_file="$(mktemp)"
        if run_sh_test "$sh_test" >"$log_file" 2>&1; then
            echo "==> PASS: $sh_test"
            rm -f "$log_file"
        else
            echo "==> FAIL: $sh_test" >&2
            cat "$log_file" >&2
            rm -f "$log_file"
            exit 1
        fi
    else
        run_sh_test "$sh_test"
    fi
done
CONTAINER_EOF

exec "$CONTAINER_ENGINE" run --rm --privileged \
    -v "$REPO_ROOT:/src:ro" \
    "$IMAGE_NAME" \
    bash -c "$CONTAINER_SCRIPT" _ \
    ${NORMALIZED_TESTS[@]+"${NORMALIZED_TESTS[@]}"}
