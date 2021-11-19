#!/bin/bash
set -e
make clean
make
set +e
sudo rmmod rtl8188
set -e
sudo modprobe cfg80211
sudo modprobe mac80211
sudo insmod ./rtl8188.ko
