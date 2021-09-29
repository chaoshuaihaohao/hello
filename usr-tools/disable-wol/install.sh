#!/bin/bash
cp -a 50-wol-disable.rules /etc/udev/rules.d/
mkdir -pv /opt/udevScripts
cp -a wol-disable.sh /opt/udevScripts/
chmod +x /opt/udevScripts/wol-disable.sh
/opt/udevScripts/wol-disable.sh
systemctl restart udev
reboot
