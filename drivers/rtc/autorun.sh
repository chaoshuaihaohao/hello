#!/bin/bash
make clean
make
sudo rmmod driver_rtc
sudo insmod driver_rtc.ko
gcc read_rtc.c
sudo ./a.out
