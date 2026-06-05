#!/bin/bash
set -ex

NS_TESTER="tester_ns_v6"
NS_TARGET="target_ns_v6"
LOG_FILE=$(mktemp /tmp/asym_trace_v6.XXXXXX.log)

cleanup() {
    echo "--- FINALER TRACE-LOG (IPv6) ---"
    [ -f "$LOG_FILE" ] && cat "$LOG_FILE"
    sudo ip netns del "$NS_TESTER" 2>/dev/null || true
    sudo ip netns del "$NS_TARGET" 2>/dev/null || true
}
trap cleanup EXIT

sudo ip netns add "$NS_TESTER"
sudo ip netns add "$NS_TARGET"

sudo ip link add veth1_tst type veth peer name veth1_trg
sudo ip link add veth2_tst type veth peer name veth2_trg

sudo ip link set veth1_tst netns "$NS_TESTER"
sudo ip link set veth2_tst netns "$NS_TESTER"
sudo ip link set veth1_trg netns "$NS_TARGET"
sudo ip link set veth2_trg netns "$NS_TARGET"

sudo ip netns exec "$NS_TESTER" ip addr add 2001:db8:1::1/64 dev veth1_tst
sudo ip netns exec "$NS_TESTER" ip addr add 2001:db8:2::1/64 dev veth2_tst
sudo ip netns exec "$NS_TARGET" ip addr add 2001:db8:1::2/64 dev veth1_trg
sudo ip netns exec "$NS_TARGET" ip addr add 2001:db8:2::2/64 dev veth2_trg

sudo ip netns exec "$NS_TESTER" ip link set veth1_tst up
sudo ip netns exec "$NS_TESTER" ip link set veth2_tst up
sudo ip netns exec "$NS_TESTER" ip link set lo up
sudo ip netns exec "$NS_TARGET" ip link set veth1_trg up
sudo ip netns exec "$NS_TARGET" ip link set veth2_trg up
sudo ip netns exec "$NS_TARGET" ip link set lo up

echo "--- Configuration for asymmetric routing (IPv6) ---"
for ns in $NS_TESTER $NS_TARGET; do
    sudo ip netns exec "$ns" sysctl -w net.ipv6.conf.all.forwarding=1
    sudo ip netns exec "$ns" sysctl -w net.ipv6.conf.all.accept_ra=0
    
    for dev in $(sudo ip netns exec "$ns" ls /sys/class/net/); do
        sudo ip netns exec "$ns" sysctl -w net.ipv6.conf.$dev.forwarding=1 2>/dev/null || true
    done
done

T_MAC2=$(sudo ip netns exec "$NS_TARGET" cat /sys/class/net/veth2_trg/address)
sudo ip netns exec "$NS_TESTER" ip -6 neigh add 2001:db8:2::2 lladdr "$T_MAC2" dev veth2_tst

echo "--- Tests ---"
sudo ip netns exec "$NS_TESTER" tcpdump -i any icmp6 -n -l > $LOG_FILE 2>&1 &
TCP_PID=$!
sleep 2

echo "Send fping6 (asymmetry check)..."
sudo ip netns exec "$NS_TESTER" "$(dirname "$0")/../src/fping" -6 -c 1 -t 1000 --oiface veth2_tst -S 2001:db8:1::1 2001:db8:2::2 || true

sleep 1
sudo kill $TCP_PID 2>/dev/null || true
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