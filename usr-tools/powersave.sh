#!/bin/bash
CPU_MODE=powersave


if [ -n $JOBS ];then
        JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`
        if [ ! $JOBS ];then
                JOBS="1"
        fi
fi
export MAKEFLAGS=-j$JOBS

for ((i = 0;i <= $JOBS - 1; i++))
do
	echo $CPU_MODE > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
	cat /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
done

#把控制中心杀掉
pkill -ef dde-control-center

#把 wifi 关掉
nmcli radio wifi off

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

#把终端关掉

#chenhao done
#关掉systemd-journal
sudo pkill -ef systemd-journal

#配置外设节能Sets all tunable options to their GOOD setting
sudo powertop --auto-tune
