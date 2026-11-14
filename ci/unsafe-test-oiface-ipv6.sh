#!/bin/bash
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: must run as root (e.g. via sudo)" >&2
    exit 1
fi

set -ex

NS_TESTER="tester_ns_v6"
NS_TARGET="target_ns_v6"
LOG_FILE=$(mktemp /tmp/asym_trace_v6.XXXXXX.log)

cleanup() {
    echo "--- FINALER TRACE-LOG (IPv6) ---"
    [ -f "$LOG_FILE" ] && cat "$LOG_FILE"
    ip netns del "$NS_TESTER" 2>/dev/null || true
    ip netns del "$NS_TARGET" 2>/dev/null || true
    rm -f "$LOG_FILE"
}
trap cleanup EXIT

ip netns add "$NS_TESTER"
ip netns add "$NS_TARGET"

ip link add veth1_tst type veth peer name veth1_trg
ip link add veth2_tst type veth peer name veth2_trg

ip link set veth1_tst netns "$NS_TESTER"
ip link set veth2_tst netns "$NS_TESTER"
ip link set veth1_trg netns "$NS_TARGET"
ip link set veth2_trg netns "$NS_TARGET"

ip netns exec "$NS_TESTER" ip addr add 2001:db8:1::1/64 dev veth1_tst
ip netns exec "$NS_TESTER" ip addr add 2001:db8:2::1/64 dev veth2_tst
ip netns exec "$NS_TARGET" ip addr add 2001:db8:1::2/64 dev veth1_trg
ip netns exec "$NS_TARGET" ip addr add 2001:db8:2::2/64 dev veth2_trg

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
    
    for dev in $(ip netns exec "$ns" ls /sys/class/net/); do
        ip netns exec "$ns" sysctl -w "net.ipv6.conf.$dev.forwarding=1" 2>/dev/null || true
    done
done

T_MAC2=$(ip netns exec "$NS_TARGET" cat /sys/class/net/veth2_trg/address)
ip netns exec "$NS_TESTER" ip -6 neigh add 2001:db8:2::2 lladdr "$T_MAC2" dev veth2_tst

echo "--- Tests ---"
ip netns exec "$NS_TESTER" tcpdump -i any icmp6 -n -l > "$LOG_FILE" 2>&1 &
TCP_PID=$!
sleep 2

echo "Send fping6 (asymmetry check)..."
ip netns exec "$NS_TESTER" "$(dirname "$0")/../src/fping" -6 -c 1 -t 1000 --oiface veth2_tst -S 2001:db8:1::1 2001:db8:2::2 || true

sleep 1
kill "$TCP_PID" 2>/dev/null || true
sleep 1

echo "--- Analysis ---"

REQ_OK=$(grep "veth2_tst Out IP6 2001:db8:1::1 > 2001:db8:2::2" "$LOG_FILE" | wc -l)
REP_OK=$(grep "veth1_tst In  IP6 2001:db8:2::2 > 2001:db8:1::1" "$LOG_FILE" | wc -l)

if [ "$REQ_OK" -gt 0 ] && [ "$REP_OK" -gt 0 ]; then
    echo "RESULT: TEST SUCCESSFUL (IPv6 True asymmetry detected)"
    exit 0
else
    echo "RESULT: TEST FAILED"
    [ "$REQ_OK" -eq 0 ] && echo "- The IPv6 request was not sent via veth2_tst."
    [ "$REP_OK" -eq 0 ] && echo "- The IPv6 reply was not received via veth1_tst."
    exit 1
fi
