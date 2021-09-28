#!/bin/bash
# Author: Hao Chen <chenhaoa@uniontech.com>
# Time	: 2021/9/23

if [ `id -u` != 0 ];then
        echo Permission delay, Please run as root!
        exit
fi

CAN_USE=false

SERVICE="systemd-journald \
	NetworkManager \
	upower.service \
	com.deepin.userexperience.Daemon.service \
	deepin-anything-tool.service \
	runsunloginclient.service \
	"

#参数检测
case $1 in
	recover )
		for service in $SERVICE
		do
			systemctl enable $service
			systemctl start $service
		done
		nmcli networking on
		nmcli radio wifi on
		exit
	;;
esac

#Download powertop tools
dpkg -s powertop > /dev/null
if [ $? -ne 0 ];then
	apt install -y powertop
fi

if [ -n $JOBS ];then
        JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`
        if [ ! $JOBS ];then
                JOBS="1"
        fi
fi
export MAKEFLAGS=-j$JOBS

#CPU调频模式更改为powersave节能模式
CPU_MODE=powersave
for ((i = 0;i <= $JOBS - 1; i++))
do
	echo $CPU_MODE > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
	cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
done

#把控制中心杀掉
pkill -ef dde-control-center

#把 wifi 关掉
nmcli radio wifi off

#关掉有线网卡
#IF_NAME=`ip ad | awk -F ': ' '/state UP/ {print $2}'`
IF_NAME=`ip ad | awk -F ': ' '/UP/ {print $2}'`
for if in $IF_NAME
do
        if [ -d /sys/class/net/$if -a ! -d /sys/class/net/$if/wireless ]
        then
                ifconfig $if down
        fi
done

#关掉有线网卡
nmcli networking off

#关掉网络管理服务
systemctl stop NetworkManager
systemctl disable NetworkManager

#把蓝牙关掉
rfkill block bluetooth

#把启动器关掉
pkill -ef dde-launcher

#把安全中心杀掉
pkill -ef deepin-defender-antiav
pkill -ef deepin-defender

#把云同步杀掉
pkill -ef deepin-sync-helper
pkill -ef deepin-deepinid-daemon

#把 upower 停掉
systemctl stop upower.service

#把用户体验计划程序停掉
systemctl stop com.deepin.userexperience.Daemon.service

#把文件搜索服务关掉
systemctl stop deepin-anything-tool.service

#把向日葵后台服务关闭
systemctl stop runsunloginclient.service

#chenhao done
#关闭虚拟键盘
pkill -ef onboard

#
pkill -ef deepin-deepinid

#关掉systemd-journal
pkill -ef systemd-journal
systemctl stop systemd-journald
systemctl disable systemd-journald

#配置外设节能Sets all tunable options to their GOOD setting
powertop --auto-tune

#关闭显示器屏幕
xset dpms force off

#把终端关掉
pkill deepin-terminal
