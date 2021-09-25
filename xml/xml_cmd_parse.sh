#!/bin/bash

#拆分xml文件
#xml_split -c screen -c userinput $1
xml_split -c screen $1
#获取xml文件名
XML_NAME=`basename -s .xml $1`
DIR_NAME=`dirname $1`
SPLIT_FILE=$DIR_NAME/$XML_NAME-0*.xml
echo $SPLIT_FILE

rm -f $XML_NAME.cmd
#筛选含有userinput关键字的xml拆分文件
for file in $SPLIT_FILE
do
	grep -q userinput $file
	if [ $? = 0 ];then
		echo $file
		./xml $file >> $XML_NAME.cmd
		#合并userinput内容到文件
		#cat $file >> tmp.txt
	fi
done

