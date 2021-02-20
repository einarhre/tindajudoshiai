#!/bin/bash

RELEASEDIR=$1
TARGETOS=$2
VER=$3
PROG=$4

echo "$RELEASEDIR $TARGETOS $VER $PROG"

rm -rf /tmp/AppDir
mkdir -p /tmp/AppDir
cp -a $RELEASEDIR/pkg/usr /tmp/AppDir/

if [ "$TARGETOS" = "LINUX" ]; then
    YML=mk/AppImageBuilder-$PROG.yml
elif [ "$TARGETOS" = "LINUX32" ]; then
    YML=mk/AppImageBuilder-$PROG-i686.yml
elif [ "$TARGETOS" = "LINUXARM" ]; then
    YML=mk/AppImageBuilder-${PROG}-armhf.yml
else
    echo "TARGETOS=$TARGETOS, exiting"
    exit 1
fi

cat $YML | \
    sed "s/latest/$VER/" | \
    sed "s,./AppDir,/tmp/AppDir," >/tmp/AppImageBuilder.yml

cd $RELEASEDIR && appimage-builder --skip-test --recipe /tmp/AppImageBuilder.yml
