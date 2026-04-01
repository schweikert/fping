set -ex

NS_TESTER="tester_ns"
NS_TARGET="target_ns"
LOG_FILE="/tmp/asym_trace.log"

cleanup() {
    echo "--- FINALER TRACE-LOG ---"
    [ -f "$LOG_FILE" ] && cat $LOG_FILE
    sudo ip netns del $NS_TESTER 2>/dev/null || true
    sudo ip netns del $NS_TARGET 2>/dev/null || true
}
trap cleanup EXIT

echo "--- Network-Setup ---"
sudo ip netns add $NS_TESTER
sudo ip netns add $NS_TARGET

sudo ip link add veth1_tst type veth peer name veth1_trg
sudo ip link add veth2_tst type veth peer name veth2_trg

sudo ip link set veth1_tst netns $NS_TESTER
sudo ip link set veth2_tst netns $NS_TESTER
sudo ip link set veth1_trg netns $NS_TARGET
sudo ip link set veth2_trg netns $NS_TARGET

sudo ip netns exec $NS_TESTER ip addr add 10.0.1.1/24 dev veth1_tst
sudo ip netns exec $NS_TESTER ip addr add 10.0.2.1/24 dev veth2_tst
sudo ip netns exec $NS_TARGET ip addr add 10.0.1.2/24 dev veth1_trg
sudo ip netns exec $NS_TARGET ip addr add 10.0.2.2/24 dev veth2_trg

sudo ip netns exec $NS_TESTER ip link set veth1_tst up
sudo ip netns exec $NS_TESTER ip link set veth2_tst up
sudo ip netns exec $NS_TESTER ip link set lo up
sudo ip netns exec $NS_TARGET ip link set veth1_trg up
sudo ip netns exec $NS_TARGET ip link set veth2_trg up
sudo ip netns exec $NS_TARGET ip link set lo up

echo "--- Configuration for asymmetric routing ---"
for ns in $NS_TESTER $NS_TARGET; do
    sudo ip netns exec $ns sysctl -w net.ipv4.conf.all.rp_filter=0
    sudo ip netns exec $ns sysctl -w net.ipv4.conf.default.rp_filter=0
    sudo ip netns exec $ns sysctl -w net.ipv4.conf.all.accept_local=1
    sudo ip netns exec $ns sysctl -w net.ipv4.ip_forward=1

    for dev in $(sudo ip netns exec $ns ls /sys/class/net/); do
        sudo ip netns exec $ns sysctl -w net.ipv4.conf.$dev.rp_filter=0 2>/dev/null || true
    done
done

T_MAC2=$(sudo ip netns exec $NS_TARGET cat /sys/class/net/veth2_trg/address)
sudo ip netns exec $NS_TESTER arp -s 10.0.2.2 $T_MAC2 -i veth2_tst

echo "--- Tests ---"
sudo ip netns exec $NS_TESTER tcpdump -i any icmp -n -l > $LOG_FILE 2>&1 &
TCP_PID=$!
sleep 2

echo "Send fping (asymmetry check)..."
sudo ip netns exec $NS_TESTER ./src/fping -c 1 -t 1000 --oiface veth2_tst -S 10.0.1.1 10.0.2.2 || FPING_STATUS=$?

sleep 1
sudo kill $TCP_PID 2>/dev/null || true
sleep 1

echo "--- Analysis ---"

REQ_OK=$(grep "veth2_tst Out IP 10.0.1.1 > 10.0.2.2" $LOG_FILE | wc -l)
REP_OK=$(grep "veth1_tst In  IP 10.0.2.2 > 10.0.1.1" $LOG_FILE | wc -l)

if [ "$REQ_OK" -gt 0 ] && [ "$REP_OK" -gt 0 ]; then
    echo "RESULT: TEST SUCCESSFUL (True asymmetry detected)"
    exit 0
else
    echo "RESULT: TEST FAILED"
    [ "$REQ_OK" -eq 0 ] && echo "- The request was not sent correctly with source 10.0.1.1 via veth2_tst."
    [ "$REP_OK" -eq 0 ] && echo "- The reply was not received asymmetrically via veth1_tst."
    exit 1
fi