#!/bin/bash

this_dir=$(dirname "$(readlink -f "$0")")
echo $this_dir

cd $this_dir

# if [ ! -f "$this_dir/appimagetool-x86_64.AppImage" ]; then
#   wget https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
# fi

# if [ ! -f "$this_dir/linuxdeploy-x86_64.AppImage" ]; then
#   wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
# fi

# if [ ! -f "$this_dir/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
#   wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
# fi

# if [ ! -d "$this_dir/qt-everywhere-src-6.9.3" ]; then
#   wget https://download.qt.io/archive/qt/6.9/6.9.3/single/qt-everywhere-src-6.9.3.tar.xz && tar -xf qt-everywhere-src-6.9.3.tar.xz && rm -f qt-everywhere-src-6.9.3.tar.xz
# fi

# docker buildx build . -t lxxl1024/almalinux-qt693-builder --build-arg QT_BUILD_NPROC=14  > build.log 2>&1 && \
cd .. && \
docker run --rm -v "$(pwd):/root/GLog" lxxl1024/almalinux-qt693-builder "/root/GLog/linux-deploy/linux-deploy.sh"
