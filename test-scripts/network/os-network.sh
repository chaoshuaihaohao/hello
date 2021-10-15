#!/bin/bash
#/*
#确认硬件正常
#	lspci正常识别
#	/sys/devices成功注册
#
#确认驱动正常
#	lspci -k获取驱动名，并显示驱动名称到终端。
#	检测/sys/drivers是否成功注册
#
#确认firmware正常
#	根据硬件pid/vid，查询modules.alias/buitin文件，得到驱动名，modinfo驱动模块名获取firmware(builtin如何获取？);
#	检测/lib/firmware/目录下的文件，判断是否缺少firmware，发出警告。
#
#确认驱动和硬件成功匹配并初始化
#	ip a显示网络接口已生成（或检测/sys/class/net/*接口文件已生成）
#
#确认网线正常连接
#	sudo ethtool $IF获取Link状态
#
#确认ip成功获取
#	ip是存在哪里的？grep -rn "ifa_local =" struct in_ifaddr { <- :struct in_device {
#
#确认网卡up后收发包统计正常，未出现rx/tx无法发包
#
#确认PHY芯片的寄存器正常
#
#确认PHY芯片的接口模式和ACPI规定的一致
#
#确认内网认证成功通过
#
#*/

LOG=./log
case "$1" in
log)
	if [ ! -d $LOG ];then
		rm -rf $LOG
		mkdir -p $LOG
	fi
	sudo lshw -c network > $LOG/lshw-network.txt
	hwinfo --network > $LOG/hwinfo-network.txt
	#pci信息
	PCI_BUS_IDs=$(lspci | grep Ethernet | awk -F ' ' '{print $1}')
	lspci > $LOG/lspci.txt
	lspci -t > $LOG/lspci-t.txt
	for PCI_BUS_ID in $PCI_BUS_IDs
	do
		sudo lspci -xxxvvv -s $PCI_BUS_ID > $LOG/lspci-vvvxxx.txt
	done
	#phy寄存器信息
IF_NAME=$(ip ad | awk -F ': ' '/state/ {print $2}')
for IF in $IF_NAME
do
	#跳过lo网口
	if [ "$IF" = "lo" ];then continue; fi
	sudo mii-tool -vvv $IF > $LOG/mii-tool.txt 2>/dev/null
	for PHY_ID in $(sudo find /sys -name phy_id)
	do
		echo "$PHY_ID" >> $LOG/mii-tool.txt
		cat "$PHY_ID" >> $LOG/mii-tool.txt
	done
done
	#ACPI信息
	sudo cp /sys/firmware/acpi/tables/DSDT $LOG
	#内核日志
	sudo cp /var/log/messages* $LOG
	sudo cp /var/log/kern.log* $LOG
	#TODO:journal日志
	exit;
	;;
clean)
	rm $LOG -rf
	exit;
	;;
*)
	;;
esac

#网络硬件配置.
#TODO:检查bios是否禁用网卡

#检查网卡硬件是否被内核识别
lspci | grep -q Ethernet
if [ $? -ne 0 ];then
	echo Error: lspci未发现Ethernet网卡硬件设备!
else
	echo lspci检测到网卡硬件设备.
fi

#网卡电源及

#网卡驱动是否正常工作(通过ip a查看是否有网络接口文件)
IF_NAME=$(ip ad | awk -F ': ' '/state/ {print $2}')
NUM=$(ip ad | awk -F ': ' '/state/ {print $2}' | wc -l)
if [ $NUM -eq 1 ];then
	echo Error: 未加载驱动!
	#通过设备id找到对应驱动,并提示用户安装下载
	PCI_BUS_IDs=$(lspci | grep Ethernet | awk -F ' ' '{print $1}')
	for PCI_BUS_ID in $PCI_BUS_IDs
	do
		VID=$(lspci -n -s $PCI_BUS_ID | awk -F ":" '{print $(NF-1)}' | awk -F ' ' '{print $1}')
		#echo $VID
		DID=$(lspci -n -s $PCI_BUS_ID | awk -F ":" '{print $NF}')
		#echo $DID
		DRIVER=$(grep -ri "$VID" /lib/modules/`uname -r`/modules.alias | grep -i "$DID" | awk -F ' ' '{print $NF}')
		echo "Please insmod the '$DRIVER' modules."
	done
fi

for IF in $IF_NAME
do
	#跳过lo网口
	if [ "$IF" = "lo" ];then continue; fi
	if [ -d /sys/class/net/$IF ];then
		#内核设备文件是否正常注册
		#echo $IF 驱动注册成功!
		#驱动文件是否匹配
		BUS_INFO=$(ethtool -i $IF | grep bus-info | awk -F ': ' '{print $2}')
		DRIVE_INFO=$(sudo find /sys -name "$BUS_INFO" | grep drivers)
		if [ $? -ne 0 ];then
			echo Error: 没有驱动匹配上 $IF device!
		else
			#DRIVER_NAME=$(echo $DRIVE_INFO | awk -F 'drivers/' '{print $2}' | awk -F '/' '{print $1}')
			DRIVER_NAME=$(ethtool -i eno1 | grep driver | awk -F ' ' '{print $NF}')
			echo 驱动 $DRIVER_NAME 匹配 $IF device 成功.
		fi
		#检查网线是否插好
		LINK=$(sudo ethtool $IF | grep 'Link detected' | awk -F ': ' '{print $NF}')
		if [ "$LINK" = "yes" ];then
			echo 网线已插好
		else
			echo Error: 网线未插好!
		fi
		#网络连接配置
		#检查网卡相关设置是否正确,IP地址是否配置正确
		#mtu是否正确
		MTU=$(ip link show "$IF" | grep mtu | awk -F 'mtu ' '{print $2}' | awk -F ' ' '{print $1}')
		if [ "$MTU" -ne 1500 ];then
			echo "Warning: MTU($MTU) 值可能异常!"
		else
			echo MTU=$MTU
		fi
		#IP是否成功配置,是否是有效IP .格式10.20.53.139/24
		IP=$(ip address show "$IF" | grep -w inet | awk -F ' ' '{print $2}')
		if [ -z $IP ];then
			echo Error: 未配置IP, IP为空!
			#无法获取ip原因有很多.比如网卡由于某些原因不能收包或不能发包(大概率).系统服务异常等.
			#检查DHCP服务是否正常工作
			sudo systemctl status isc-dhcp-server.service > /dev/null
			if [ $? -ne 0 ];then
				echo Error: DHCP service 未正常运行!
			else
				echo DHCP service 正常运行.
			fi
			#检查NetworkManager服务是否正常工作(这个服务状态正常也有概率造成无法认证上网)
			sudo systemctl status NetworkManager > /dev/null
			if [ $? -ne 0 ];then
				echo Error: NetworkManager service 未正常运行!
			else
				echo NetworkManager service 正常运行.
			fi
			#检查NetworkManager是否关闭网络
			CONNECTION=$(nmcli device | grep "$IF" | awk -F ' ' '{print $4}')
			if [ "$CONNECTION" = "--" ];then
				echo Warning: NetworkManager close the interface, please up it!
			fi
		else
			IP_ADDR=$(echo $IP | awk -F '/' '{print $1}')
			#TODO: check the ip validate
			echo IP ADDR=$IP_ADDR
			IP_MASK=$(echo $IP | awk -F '/' '{print $2}')
			echo IP MASK=$IP_MASK
			#IP获取成功但是无法上网.
			#可能是无法ping通网关
			GATEWAY=$(ip route show | head -1 | awk -F ' ' '{print $3}')
			if [ -z "$GATEWAY" ];then
				echo Error: gateway not configured!
			elif [ $(echo $GATEWAY | awk -F '.' '{print $4}') = "0" ];then
				echo "Error: gateway($GATEWAY) is not right! \
					Maybe you need authentication or restart the NetworkManager service!"
			else
				ping -c 1 $GATEWAY -W 2 > /dev/null
				if [ $? -ne 0 ];then
					echo "Error: 不能ping通网关($GATEWAY)!"
				else
					echo "可以ping通网关($GATEWAY)."
				fi
			fi
			#也可能是认证问题
			#检查网页认证是否ok
			#curl -w "@curl-format.txt" -o /dev/null -s -L "https://baidu.com"
			curl www.baidu.com > /dev/null 2>&1
			if [ $? -ne 0 ];then
				echo Error: curl 未认证!
			else
				echo 网络正常.
			fi

		fi
	else
		echo Error: 驱动未成功注册!
	fi

	echo ""
done


