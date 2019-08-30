#!/bin/sh

sysctl_file_name=/etc/sysctl.conf
max_open_files=1000000
file_max_str="fs.file-max = ${max_open_files}"
ip_local_port_range_str="net.ipv4.ip_local_port_range = 1024 65535"

old_file_max_str=`cat ${sysctl_file_name} | grep "${file_max_str}"`
if [ "${old_file_max_str}" != "${file_max_str}" ]; then
  echo "${file_max_str}" >> ${sysctl_file_name}
fi

old_ip_local_port_range_str=`cat ${sysctl_file_name} | grep "${ip_local_port_range_str}"`
if [ "${old_ip_local_port_range_str}" != "${ip_local_port_range_str}" ]; then
  echo "${ip_local_port_range_str}" >> ${sysctl_file_name}
fi

sysctl -p

limits_file_name=/etc/security/limits.conf
user_soft_nofile_str="*    soft nofile ${max_open_files}"
user_hard_nofile_str="*    hard nofile ${max_open_files}"
root_soft_nofile_str="root soft nofile ${max_open_files}"
root_hard_nofile_str="root hard nofile ${max_open_files}"

old_user_soft_nofile_str=`cat ${limits_file_name} | grep "${user_soft_nofile_str}"`
if [ "${old_user_soft_nofile_str}" != "${user_soft_nofile_str}" ]; then
  echo "${user_soft_nofile_str}" >> ${limits_file_name}
fi

old_user_hard_nofile_str=`cat ${limits_file_name} | grep "${user_hard_nofile_str}"`
if [ "${old_user_hard_nofile_str}" != "${user_hard_nofile_str}" ]; then
  echo "${user_hard_nofile_str}" >> ${limits_file_name}
fi

old_root_soft_nofile_str=`cat ${limits_file_name} | grep "${root_soft_nofile_str}"`
if [ "${old_root_soft_nofile_str}" != "${root_soft_nofile_str}" ]; then
  echo "${root_soft_nofile_str}" >> ${limits_file_name}
fi

old_root_hard_nofile_str=`cat ${limits_file_name} | grep "${root_hard_nofile_str}"`
if [ "${old_root_hard_nofile_str}" != "${root_hard_nofile_str}" ]; then
  echo "${root_hard_nofile_str}" >> ${limits_file_name}
fi

ulimit -n ${max_open_files}
