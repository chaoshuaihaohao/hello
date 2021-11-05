#!/bin/bash
make clean
make
sudo rmmod device_rtc
sudo insmod device_rtc.ko
sudo rmmod driver_rtc
sudo insmod driver_rtc.ko
gcc read_rtc.c
sudo ./a.out
