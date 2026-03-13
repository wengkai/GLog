#!/bin/bash

# run this script in a docker container
# see workflow.sh 

. /etc/profile && \
mkdir -p /root/GLog/build-linux &&  \
cd /root/GLog/build-linux &&  \
rm -rf * &&  \
. $ENABLE_GCC_TOOLSET &&  \
qt-cmake .. -DCMAKE_INSTALL_PREFIX=/usr &&  \
make -j$(nproc) && \
make install DESTDIR=$PWD/AppDir &&  \
linuxdeploy --appdir AppDir -d ../linux-deploy/GLog.desktop -e AppDir/usr/bin/GLog -i ../linux-deploy/GLog.png --plugin=qt --output appimage
