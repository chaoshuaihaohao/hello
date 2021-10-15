#!/bin/bash


#检测显卡硬件
#	lshw -c vedio
#	独显还是集显还是核显：dmidecode -t slot查看pcie插槽，插卡的为独显
#
#

#*/

sudo lshw -c video
