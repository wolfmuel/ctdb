#!/bin/bash

test_info()
{
    cat <<EOF
Verify that a gratuitous ARP is sent when a node is failed out.

We ping a public IP and lookup the MAC address in the ARP table.  We
then disable the node and check the ARP table again - the MAC address
should have changed.  This test does NOT test connectivity after the
failover.

Prerequisites:

* An active CTDB cluster with at least 2 nodes with public addresses.

* Test must be run on a real or virtual cluster rather than against
  local daemons.

* Test must not be run from a cluster node.

Steps:

1. Verify that the cluster is healthy.
2. Select a public address and its corresponding node.
3. Remove any entries for the chosen address from the ARP table.
4. Send a single ping request packet to the selected public address.
5. Determine the MAC address corresponding to the public address by
   checking the ARP table.
6. Disable the selected node.
7. Check the ARP table and check the MAC associated with the public
   address.

Expected results:

* When a node is disabled the MAC address associated with public
  addresses on that node should change.
EOF
}

. ctdb_test_functions.bash

set -e

ctdb_test_init "$@"

ctdb_test_check_real_cluster

cluster_is_healthy

# Reset configuration
ctdb_restart_when_done

select_test_node_and_ips

echo "Removing ${test_ip} from the local ARP table..."
arp -d $test_ip >/dev/null 2>&1 || true

echo "Pinging ${test_ip}..."
ping -q -n -c 1 $test_ip

echo "Getting MAC address associated with ${test_ip}..."
original_mac=$(arp -n $test_ip | awk '$2 == "ether" {print $3}')
[ $? -eq 0 ]

echo "MAC address is: ${original_mac}"

gratarp_sniff_start

echo "Disabling node $test_node"
try_command_on_node 1 $CTDB disable -n $test_node
wait_until_node_has_status $test_node disabled

gratarp_sniff_wait_show

echo "Getting MAC address associated with ${test_ip} again..."
new_mac=$(arp -n $test_ip | awk '$2 == "ether" {print $3}')
[ $? -eq 0 ]

echo "MAC address is: ${new_mac}"

if [ "$original_mac" != "$new_mac" ] ; then
    echo "GOOD: MAC address changed"
else
    echo "BAD: MAC address did not change"
    testfailures=1
fi
