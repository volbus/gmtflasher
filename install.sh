#!/bin/bash
# Written by C.G.:	18-03-2021
# last modified:	12-04-2021

#this is automatic version extraction from git tags, but, to make it simple,
#just edit the version.h file manually and keep the following commented
 #git tag -l --sort=-version:refname >> /tmp/gmtflasher_project_tags
 #set_project_version /tmp/gmtflasher_project_tags version.h
 #if [ $? -ne 0 ]; then
 #	rm /tmp/gmtflasher_project_tags
 #	exit
 #fi
 #rm /tmp/gmtflasher_project_tags


BASE_FLAGS="-Wall -o2 -std=gnu99"

LIB_USB_FLAGS=`pkg-config --cflags --libs libusb-1.0`
LIB_XML_FLAGS=`xml2-config --cflags --libs`

gcc $BASE_FLAGS gmtflasher.c -o /usr/local/bin/gmtflasher $LIB_USB_FLAGS $LIB_XML_FLAGS

#check existance of /usr/share/gmtflasher folder, create if needed
if [ ! -d "/usr/share/gmtflasher" ]; then
	mkdir /usr/share/gmtflasher
	if [ $? -ne 0 ]; then
		exit
	fi
fi

#copy the mcu definition file
cp -u gmtflasher_devices.xml /usr/share/gmtflasher/
