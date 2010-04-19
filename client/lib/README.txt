
How to install ctdb
===================
Install the ctdb rpm on a node (preferably 64-bit RHEL 5.4).
Create file /etc/ctdb/nodes and put a line with the IP address of node in it.
Now a "service ctdb start" should work and ctdb is running
You may add as many nodes as you like but it works with just one node as well.
In order to add more nodes you have to adapt the /etc/ctdb/nodes file
accordingly and (re-)start ctdb on every node.

How to test if the ctdbapi lib works
====================================
Untar the libctdbapi tar file.
Make libctdbapi.so available either by putting it in a "well-known" directory or
set LD_LIBRARY_PATH.

Now by executing "ctdbtool test.tdb store key value" you should see this:
    [root@rhel5vm lib]# ./ctdbtool test.tdb store key value
    stored key(key) with value(value)
 
and you can fetch the value by executing "ctdbtool test.tdb fetch key":
    [root@rhel5vm lib]# ./ctdbtool test.tdb fetch key
    key(key) => value(value)

If ctdb has not been started yet you'll get an error like this:
   [root@rhel5vm lib]# ./ctdbtool test.db store key value
   2010/04/18 15:56:59.109807 [24862]: client/ctdb_client.c:278 Failed to connect client socket to daemon. Errno:Connection refused(111)
   common/cmdline.c:149 Failed to connect to daemon




