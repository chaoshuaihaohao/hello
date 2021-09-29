#!/bin/bash
IF_NAME=`ip ad | awk -F ': ' '/state/ {print $2}'`

for file in $IF_NAME
do
	if [ "$file" != "lo" ]
	then
		ethtool -s $file wol d
	fi
done
