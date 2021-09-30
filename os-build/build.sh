#!/bin/bash
LFS_PATH=/home/uos/Backup/github/lfs-git/
BUILD_PATH=./build
CMD_PATH=$BUILD_PATH/cmd
PKG_PATH=$BUILD_PATH/pkg
SRC_PATH=$BUILD_PATH/
make -C ./xml_parse
XML_PARSE=./xml_parse/xml

CUR_PATH=$(dirname $(readlink -f "$0"))
#echo $CUR_PATH

XML_FILE_SET=`find $LFS_PATH -name "*.xml"`

mkdir -pv $BUILD_PATH

#生成.cmd文件
rm -rf $CMD_PATH
mkdir -pv $CMD_PATH
for file in $XML_FILE_SET
do
	XML_NAME=$(basename $file)
	BASENAME=$(basename -s .xml $XML_NAME)
	$XML_PARSE -n cmd $file > $CMD_PATH/$BASENAME.cmd
	if [ ! -s $CMD_PATH/$BASENAME.cmd ]
	then
		rm $CMD_PATH/$BASENAME.cmd
	fi
	echo $XML_PARSE $file

done

#生成.pkg文件,包含包的信息
rm -rf $PKG_PATH
mkdir -pv $PKG_PATH

#找到包含软件包下载信息的文件
PKG_FILE=`find $LFS_PATH -name packages.xml`
$XML_PARSE -n pkg $PKG_FILE > $PKG_PATH/$(basename -s .xml $PKG_FILE).pkg

#找到包含patches下载信息的文件
PATCH_FILE=`find $LFS_PATH -name patches.xml`
$XML_PARSE -n pkg $PATCH_FILE > $PKG_PATH/$(basename -s .xml $PATCH_FILE).pkg

#wget下载软件包url
mkdir -pv $SRC_PATH
#获取安装包
PACKAGES=`grep -r Download $PKG_PATH/$(basename -s .xml $PKG_FILE).pkg | awk -F ': ' '{print $2}'`
for pkg in $PACKAGES
do
	#-N:只获取比本地文件新的文件,避免重复下载
	wget -N -c $pkg -P $SRC_PATH
done
#获取patch
PACKAGES=`grep -r Download $PKG_PATH/$(basename -s .xml $PATCH_FILE).pkg | awk -F ': ' '{print $2}'`
for pkg in $PACKAGES
do
	#-N:只获取比本地文件新的文件,避免重复下载
	wget -N -c $pkg -P $SRC_PATH
done

#rm -rf $PKG_PATH
#rm -rf $CMD_PATH
#rm -rf $BUILD_PATH
make -C ./xml_parse clean

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@build
#解压软件包
tar xf 
pushd 




