if [ "$(id -u)" -ne 0 ]; then
    echo "Error: must run as root (e.g. via sudo)" >&2
    exit 1
fi

set -ex

NS_TESTER="tester_ns"
NS_TARGET="target_ns"
LOG_FILE=$(mktemp /tmp/asym_trace.XXXXXX.log)

cleanup() {
    echo "--- FINALER TRACE-LOG ---"
    [ -f "$LOG_FILE" ] && cat "$LOG_FILE"
    rm -f "$LOG_FILE"
    ip netns del "$NS_TESTER" 2>/dev/null || true
    ip netns del "$NS_TARGET" 2>/dev/null || true
}
trap cleanup EXIT

echo "--- Network-Setup ---"
ip netns add "$NS_TESTER"
ip netns add "$NS_TARGET"

ip link add veth1_tst type veth peer name veth1_trg
ip link add veth2_tst type veth peer name veth2_trg

ip link set veth1_tst netns "$NS_TESTER"
ip link set veth2_tst netns "$NS_TESTER"
ip link set veth1_trg netns "$NS_TARGET"
ip link set veth2_trg netns "$NS_TARGET"

ip netns exec "$NS_TESTER" ip addr add 10.0.1.1/24 dev veth1_tst
ip netns exec "$NS_TESTER" ip addr add 10.0.2.1/24 dev veth2_tst
ip netns exec "$NS_TARGET" ip addr add 10.0.1.2/24 dev veth1_trg
ip netns exec "$NS_TARGET" ip addr add 10.0.2.2/24 dev veth2_trg

ip netns exec "$NS_TESTER" ip link set veth1_tst up
ip netns exec "$NS_TESTER" ip link set veth2_tst up
ip netns exec "$NS_TESTER" ip link set lo up
ip netns exec "$NS_TARGET" ip link set veth1_trg up
ip netns exec "$NS_TARGET" ip link set veth2_trg up
ip netns exec "$NS_TARGET" ip link set lo up

echo "--- Configuration for asymmetric routing ---"
for ns in "$NS_TESTER" "$NS_TARGET"; do
    ip netns exec "$ns" sysctl -w net.ipv4.conf.all.rp_filter=0
    ip netns exec "$ns" sysctl -w net.ipv4.conf.default.rp_filter=0
    ip netns exec "$ns" sysctl -w net.ipv4.conf.all.accept_local=1
    ip netns exec "$ns" sysctl -w net.ipv4.ip_forward=1

    for dev in $(ip netns exec "$ns" ls /sys/class/net/); do
        ip netns exec "$ns" sysctl -w "net.ipv4.conf.$dev.rp_filter=0" 2>/dev/null || true
    done
done

T_MAC2=$(ip netns exec "$NS_TARGET" cat /sys/class/net/veth2_trg/address)
ip netns exec "$NS_TESTER" arp -s 10.0.2.2 "$T_MAC2" -i veth2_tst

echo "--- Tests ---"
ip netns exec "$NS_TESTER" tcpdump -i any icmp -n -l > "$LOG_FILE" 2>&1 &
TCP_PID=$!
sleep 2

echo "Send fping (asymmetry check)..."
ip netns exec "$NS_TESTER" ./src/fping -c 1 -t 1000 --oiface veth2_tst -S 10.0.1.1 10.0.2.2 || FPING_STATUS=$?

sleep 1
kill "$TCP_PID" 2>/dev/null || true
sleep 1

echo "--- Analysis ---"

REQ_OK=$(grep "veth2_tst Out IP 10.0.1.1 > 10.0.2.2" "$LOG_FILE" | wc -l)
REP_OK=$(grep "veth1_tst In  IP 10.0.2.2 > 10.0.1.1" "$LOG_FILE" | wc -l)

if [ "$REQ_OK" -gt 0 ] && [ "$REP_OK" -gt 0 ]; then
    echo "RESULT: TEST SUCCESSFUL (True asymmetry detected)"
    exit 0
else
    echo "RESULT: TEST FAILED"
    [ "$REQ_OK" -eq 0 ] && echo "- The request was not sent correctly with source 10.0.1.1 via veth2_tst."
    [ "$REP_OK" -eq 0 ] && echo "- The reply was not received asymmetrically via veth1_tst."
    exit 1
fi
