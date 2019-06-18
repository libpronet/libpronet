#!/bin/sh

max_open_files=1000000

echo "\nSET [file-max]"
sysctl -w fs.file-max=${max_open_files}
echo "GET [file-max]"
cat /proc/sys/fs/file-max
echo "GET [file-nr]"
cat /proc/sys/fs/file-nr

echo "\nSET [nr_open]"
sysctl -w fs.nr_open=${max_open_files}
echo "GET [nr_open]"
cat /proc/sys/fs/nr_open

echo "\nSET [ulimit -n]"
echo "${max_open_files}"
ulimit -n ${max_open_files}
echo "GET [ulimit -n]"
ulimit -n

echo "\nSET [ip_local_port_range]"
sysctl -w net.ipv4.ip_local_port_range="1024 65535"
echo "GET [ip_local_port_range]"
cat /proc/sys/net/ipv4/ip_local_port_range

echo ""
