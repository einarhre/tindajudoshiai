#!/bin/sh
DIR=/opt
echo -n "Installation directory [$DIR]: "
read answer
if [ "X$answer" != "X" ] ; then
    DIR=$answer
fi
echo "Installation directory $DIR"

if [ ! -d $DIR ] ; then
    echo -n "Directory does not exist, create? [Y/n] "
    read answer
    case $answer in
	[Nn] ) exit 0 ;;
    esac
    mkdir -p $DIR
fi

cp -r judoshiai $DIR
