#!/bin/bash
#网络硬件配置.
#检查网线是否插好
LINK=$(sudo ethtool eno1 | grep 'Link detected' | awk -F ': ' '{print $NF}')
if [ "$LINK" = "yes" ];then
	echo 网线已插好
else
	echo 网线未插好
fi
#网卡电源及

#检查bios是否禁用网卡

#网卡驱动是否正常工作(通过ip a查看是否有网络接口文件)
IF_NAME=$(ip ad | awk -F ': ' '/state/ {print $2}')
for IF in $IF_NAME
do
	#跳过lo网口
	if [ "$IF" = "lo" ];then continue; fi
	if [ -d /sys/class/net/$IF ];then
		#设备文件是否正常注册
		echo $IF device register successed!
		#驱动文件是否匹配
		BUS_INFO=$(ethtool -i $IF | grep bus-info | awk -F ': ' '{print $2}')
		DRIVE_INFO=$(sudo find /sys -name "$BUS_INFO" | grep drivers)
		if [ $? -ne 0 ];then
			echo Error: not driver attach the $IF device!
		else
			DRIVER_NAME=$(echo $DRIVE_INFO | awk -F 'drivers/' '{print $2}' | awk -F '/' '{print $1}')
			echo $DRIVER_NAME attach the $IF device.
		fi
		#网络连接配置
		#检查网卡相关设置是否正确,IP地址是否配置正确
		#mtu是否正确
		MTU=$(ip link show "$IF" | grep mtu | awk -F 'mtu ' '{print $2}' | awk -F ' ' '{print $1}')
		if [ "$MTU" -ne 1500 ];then
			echo "Error: MTU($MTU) maybe abnormal!"
		else
			echo MTU=$MTU
		fi
		#IP是否成功配置,是否是有效IP .格式10.20.53.139/24
		IP=$(ip address show "$IF" | grep -w inet | awk -F ' ' '{print $2}')
		if [ -z $IP ];then
			echo Error: no ip configured!
			#无法获取ip原因有很多.比如网卡由于某些原因不能收包或不能发包(大概率).系统服务异常等.
			#检查DHCP服务是否正常工作
			sudo systemctl status isc-dhcp-server.service > /dev/null
			if [ $? -ne 0 ];then
				echo Error: DHCP service not run actived!
			else
				echo DHCP service run actived
			fi
			#检查NetworkManager服务是否正常工作(这个服务状态正常也有概率造成无法认证上网)
			sudo systemctl status NetworkManager > /dev/null
			if [ $? -ne 0 ];then
				echo Error: NetworkManager service not run actived!
			else
				echo NetworkManager service run actived
			fi
			#检查桌面是否关闭网络
			#检查网页认证是否ok
			#curl -w "@curl-format.txt" -o /dev/null -s -L "https://baidu.com"
			curl www.baidu.com
			if [ $? -ne 0 ];then
				echo Error: not authentication!
			else
				echo Can connect to Internet.
			fi
		else
			IP_ADDR=$(echo $IP | awk -F '/' '{print $1}')
			#TODO: check the ip validate
			echo IP ADDR=$IP_ADDR
			IP_MASK=$(echo $IP | awk -F '/' '{print $2}')
			echo IP MASK=$IP_MASK
		fi
	else
		echo Error: no interface found!
	fi
done


