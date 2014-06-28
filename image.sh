#!/bin/bash -ex
#
# image.sh
# 
# Script to create a bootable bios update image for the pcengines apu board.
# The image is based on tiny core linux and is using flashrom.
#
# Copyright (c) 2014 by Byteworks GmbH
# All rights reserved
#
# http://www.byteworks.ch/
#

ROOT_DIR=$(cd $(dirname $0); pwd)

# Create temp directory
TEMP_DIR=$(mktemp -d)
trap "
 	err=\$? ;

	cd ;
	mount | grep $TEMP_DIR && umount $TEMP_DIR 2>/dev/null ;
	rm -rf $TEMP_DIR ;
	[ \$err -ne 0 ] && rm -f $ROOT_DIR/build/image.img ;
	exit $err
" 0 1 2 3 15

# Create image
dd if=/dev/zero of=$ROOT_DIR/build/image.img bs=1024k count=20
mkfs.fat $ROOT_DIR/build/image.img 

# Mount image
mount -o loop $ROOT_DIR/build/image.img $TEMP_DIR

# Copy files to image
pushd $TEMP_DIR > /dev/null

cd $TEMP_DIR
cp $ROOT_DIR/util/image/* .
cp $ROOT_DIR/build/coreboot.rom bios.rom

popd > /dev/null

# Umount image
umount $TEMP_DIR

# Make image bootable
syslinux $ROOT_DIR/build/image.img

