#!/bin/bash

cd $(dirname "$(readlink -f "$0")")

# wget https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage  && \
# wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage  && \
# wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage  && \
# wget https://download.qt.io/archive/qt/6.9/6.9.3/single/qt-everywhere-src-6.9.3.tar.xz && tar -xf qt-everywhere-src-6.9.3.tar.xz && rm -f qt-everywhere-src-6.9.3.tar.xz  && \
# docker build . -t lxxl1024/almalinux-qt-builder > build.log 2>&1 && \
cd .. && \
docker run --rm -v "$(pwd):/root/GLog" lxxl1024/almalinux-qt-builder "/root/GLog/linux-deploy/linux-deploy.sh"
