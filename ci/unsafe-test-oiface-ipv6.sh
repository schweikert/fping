#!/bin/bash
set -euo pipefail

# Mutates network namespaces and sysctls; fail closed unless explicitly opted in.
if [ "${FPING_RUN_UNSAFE_TESTS:-}" != "1" ]; then
    echo "Error: unsafe netns test disabled; run via ci/run-tests-podman.sh or set FPING_RUN_UNSAFE_TESTS=1 in a VM/container." >&2
    exit 1
fi
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: must run as root (e.g. via sudo)" >&2
    exit 1
fi

REPO_ROOT="$(cd -- "$(dirname -- "$0")/.." && pwd)"
FPING_BIN="${FPING_BIN:-$REPO_ROOT/src/fping}"
[ -x "$FPING_BIN" ] || { echo "Error: fping binary not found or not executable: $FPING_BIN" >&2; exit 1; }

set -x

NS_TAG="$$_$(date +%s)"
NS_TESTER="fping_tst_v6_$NS_TAG"
NS_TARGET="fping_trg_v6_$NS_TAG"
TCP_PID=""
LOG_FILE=$(mktemp "${TMPDIR:-/tmp}/asym_trace_v6.XXXXXX.log")

cleanup() {
    set +x
    set +e
    trap '' HUP INT QUIT TERM
    if [ -n "$TCP_PID" ]; then
        kill "$TCP_PID" 2>/dev/null || true
        wait "$TCP_PID" 2>/dev/null || true
    fi
    echo "--- FINALER TRACE-LOG (IPv6) ---"
    [ ! -f "$LOG_FILE" ] || cat "$LOG_FILE"
    rm -f "$LOG_FILE"
    ip netns del "$NS_TESTER" 2>/dev/null || true
    ip netns del "$NS_TARGET" 2>/dev/null || true
}
trap cleanup EXIT
trap 'exit 1' HUP INT QUIT TERM

ip netns add "$NS_TESTER"
ip netns add "$NS_TARGET"

for ns in "$NS_TESTER" "$NS_TARGET"; do
    ip netns exec "$ns" sysctl -w net.ipv6.conf.all.disable_ipv6=0
    ip netns exec "$ns" sysctl -w net.ipv6.conf.default.disable_ipv6=0
    ip netns exec "$ns" sysctl -w net.ipv6.conf.all.accept_dad=0
    ip netns exec "$ns" sysctl -w net.ipv6.conf.default.accept_dad=0
done

# Create veth pairs inside NS_TESTER so nothing touches the host root netns.
ip netns exec "$NS_TESTER" ip link add veth1_tst type veth peer name veth1_trg
ip netns exec "$NS_TESTER" ip link add veth2_tst type veth peer name veth2_trg

ip netns exec "$NS_TESTER" ip link set veth1_trg netns "$NS_TARGET"
ip netns exec "$NS_TESTER" ip link set veth2_trg netns "$NS_TARGET"

ip netns exec "$NS_TESTER" ip addr add 2001:db8:1::1/64 dev veth1_tst nodad
ip netns exec "$NS_TESTER" ip addr add 2001:db8:2::1/64 dev veth2_tst nodad
ip netns exec "$NS_TARGET" ip addr add 2001:db8:1::2/64 dev veth1_trg nodad
ip netns exec "$NS_TARGET" ip addr add 2001:db8:2::2/64 dev veth2_trg nodad

ip netns exec "$NS_TESTER" ip link set veth1_tst up
ip netns exec "$NS_TESTER" ip link set veth2_tst up
ip netns exec "$NS_TESTER" ip link set lo up
ip netns exec "$NS_TARGET" ip link set veth1_trg up
ip netns exec "$NS_TARGET" ip link set veth2_trg up
ip netns exec "$NS_TARGET" ip link set lo up

echo "--- Configuration for asymmetric routing (IPv6) ---"
for ns in "$NS_TESTER" "$NS_TARGET"; do
    ip netns exec "$ns" sysctl -w net.ipv6.conf.all.forwarding=1
    ip netns exec "$ns" sysctl -w net.ipv6.conf.all.accept_ra=0

    # Enumerate links via ip rather than /sys/class/net, which reads host sysfs in containers.
    for dev in $(ip netns exec "$ns" ip -o link show | awk -F': ' '{print $2}' | cut -d@ -f1); do
        ip netns exec "$ns" sysctl -w "net.ipv6.conf.$dev.forwarding=1" 2>/dev/null || true
    done
done

T_MAC2=$(ip netns exec "$NS_TARGET" ip -o link show dev veth2_trg | sed -n 's/.*link\/ether \([0-9a-f:]*\).*/\1/p')
[ -n "$T_MAC2" ] || { echo "Error: failed to resolve MAC address for veth2_trg" >&2; exit 1; }
ip netns exec "$NS_TESTER" ip -6 neigh replace 2001:db8:2::2 lladdr "$T_MAC2" nud permanent dev veth2_tst

echo "--- Tests ---"
ip netns exec "$NS_TESTER" tcpdump --immediate-mode -i any icmp6 -n -l < /dev/null > "$LOG_FILE" 2>&1 &
TCP_PID=$!
i=0
while ! grep -qF "listening on" "$LOG_FILE" 2>/dev/null; do
    kill -0 "$TCP_PID" 2>/dev/null || { echo "Error: tcpdump exited prematurely" >&2; exit 1; }
    i=$((i + 1))
    [ "$i" -lt 50 ] || { echo "Error: timed out waiting for tcpdump to start" >&2; exit 1; }
    sleep 0.1
done

echo "Send fping6 (asymmetry check)..."
FPING_STATUS=0
ip netns exec "$NS_TESTER" "$FPING_BIN" -6 -c 1 -t 1000 --oiface veth2_tst -S 2001:db8:1::1 2001:db8:2::2 || FPING_STATUS=$?

sleep 0.5
kill "$TCP_PID" 2>/dev/null || true
wait "$TCP_PID" 2>/dev/null || true
TCP_PID=""

echo "--- Analysis ---"

REQ_OK=0
REP_OK=0
if grep -qF "veth2_tst Out IP6 2001:db8:1::1 > 2001:db8:2::2" "$LOG_FILE"; then REQ_OK=1; fi
if grep -qF "veth1_tst In  IP6 2001:db8:2::2 > 2001:db8:1::1" "$LOG_FILE"; then REP_OK=1; fi

if [ "$FPING_STATUS" -eq 0 ] && [ "$REQ_OK" -eq 1 ] && [ "$REP_OK" -eq 1 ]; then
    echo "RESULT: TEST SUCCESSFUL (IPv6 True asymmetry detected)"
    exit 0
else
    echo "RESULT: TEST FAILED"
    if [ "$FPING_STATUS" -ne 0 ]; then echo "- fping6 exited with status $FPING_STATUS."; fi
    if [ "$REQ_OK" -eq 0 ]; then echo "- The IPv6 request was not sent via veth2_tst."; fi
    if [ "$REP_OK" -eq 0 ]; then echo "- The IPv6 reply was not received via veth1_tst."; fi
    exit 1
fi
