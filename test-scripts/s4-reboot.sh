#!/bin/bash
while true
do
	if [ -d /sys/class/net/`ip ad | awk -F ': ' '/state UP/ {print $2}'` ]
	then
		sleep 20
		echo reboot > /sys/power/disk	#休眠后自启动
		echo disk > /sys/power/state	#disk改成mem可以测试待机
	fi
done
