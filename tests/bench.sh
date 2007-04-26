#!/bin/sh

killall -q ctdb_bench

NUMNODES=2
if [ $# -gt 0 ]; then
    NUMNODES=$1
fi

rm -f nodes.txt
for i in `seq 1 $NUMNODES`; do
  echo 127.0.0.$i:9001 >> nodes.txt
done

killall -9 ctdb_bench
echo "Trying $NUMNODES nodes"
for i in `seq 1 $NUMNODES`; do
  $VALGRIND bin/ctdb_bench --nlist nodes.txt --listen 127.0.0.$i:9001 --socket /tmp/ctdb.127.0.0.$i $* &
done

wait
