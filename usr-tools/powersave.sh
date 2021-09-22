#!/bin/bash
if [ `id -u` != 0 ];then
        echo Permission delay, Please run as root!
        exit
fi

CAN_USE=false


#>>>>>>recover
if [ "$1" = "recover" ]
then
	systemctl enable NetworkManager
	systemctl start NetworkManager
	nmcli networking on
	nmcli radio wifi on
	exit
fi

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

#关掉有线网
nmcli networking off

#关掉网络管理服务
systemctl stop NetworkManager
systemctl disable NetworkManager

#把蓝牙关掉
rfkill block bluetooth

#把启动器关掉
pkill -ef dde-launcher

#把安全中心杀掉
sudo pkill -ef deepin-defender-antiav
#sudo pkill -ef deepin-defender

#把云同步杀掉
sudo pkill -ef deepin-sync-helper
pkill -ef deepin-deepinid-daemon

#把 upower 停掉
sudo systemctl stop upower.service

#把用户体验计划程序停掉
sudo systemctl stop com.deepin.userexperience.Daemon.service

#把文件搜索服务关掉
sudo systemctl stop deepin-anything-tool.service

#把向日葵后台服务关闭
sudo systemctl stop runsunloginclient.service

#chenhao done
#关掉systemd-journal
sudo pkill -ef systemd-journal

#配置外设节能Sets all tunable options to their GOOD setting
sudo powertop --auto-tune

#关闭显示器屏幕
xset dpms force off

#把终端关掉
pkill deepin-terminal
