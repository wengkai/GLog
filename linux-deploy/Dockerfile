FROM almalinux:8 AS build0

ENV QT_VERSION=6.9.3
ENV QT_VERSION_MAJOR=6
ENV QT_DIR=/opt/Qt${QT_VERSION}
ENV BISON_VERSION=3.8.2
ENV ENABLE_GCC_TOOLSET=/opt/rh/gcc-toolset-14/enable
ENV BUILD_ARCH=x86_64

RUN dnf install -y dnf-plugins-core && \
    dnf config-manager --set-enabled powertools && \
    dnf install -y epel-release && \
	dnf install --nogpgcheck https://mirrors.rpmfusion.org/free/el/rpmfusion-free-release-8.noarch.rpm -y && \
    dnf makecache

RUN dnf install -y file cmake ninja-build git wget \
    gcc-toolset-14 gcc-toolset-14-gcc gcc-toolset-14-gcc-c++ gcc-toolset-14-libstdc++-devel \
    libxcb-devel libxkbcommon-devel libxkbcommon-x11-devel xcb-util-devel xcb-util-image-devel \
    libX11-devel libXext-devel mesa-libGL-devel mesa-libGLU-devel xcb-util-keysyms-devel \
    fontconfig-devel freetype-devel libpng-devel libjpeg-turbo-devel xcb-util-renderutil-devel libXcursor-devel libXfixes-devel \
    sqlite-devel libzstd-devel libicu-devel glib2-devel xcb-util-wm-devel libXrender-devel xcb-util-cursor-devel hunspell hunspell-*.noarch hunspell-devel \
	systemd-devel libb2-devel libproxy-devel at-spi2-core-devel mtdev-devel tslib-devel gtk3-devel libinput-devel \
	openssl3 openssl3-devel pcre2-devel harfbuzz-devel vulkan-headers mesa-libgbm-devel double-conversion-devel libwebp-devel libmng-devel glibc-langpack-en \
	protobuf-devel clang-devel \
    flex flex-devel m4 \
    && dnf clean all && rm -rf /var/cache/dnf/*
	
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

WORKDIR /root

RUN wget https://ftp.gnu.org/gnu/bison/bison-${BISON_VERSION}.tar.gz \ 
    && tar -xzf bison-${BISON_VERSION}.tar.gz && rm -f bison-${BISON_VERSION}.tar.gz && cd bison-${BISON_VERSION} \
	&& . ${ENABLE_GCC_TOOLSET} && ./configure && make -j$(nproc) && make install && cd .. && rm -rf bison-${BISON_VERSION}

FROM build0 AS qtbuilderbase

# https://download.qt.io/archive/qt/6.9/6.9.3/single/qt-everywhere-src-6.9.3.tar.xz
COPY qt-everywhere-src-${QT_VERSION} /Qt${QT_VERSION}
WORKDIR /Qt${QT_VERSION}
RUN . ${ENABLE_GCC_TOOLSET} && \
    ./configure -prefix ${QT_DIR} -release -nomake examples -nomake tests \
    -skip qtwebengine,qtopcua,qtwayland,qtmultimedia,qtspeech -xcb -opengl desktop -openssl-runtime  \
    --verbose \
	-- -DOPENSSL_INCLUDE_DIR=/usr/include/openssl3 \
    -DOPENSSL_CRYPTO_LIBRARY=/usr/lib64/openssl3/libcrypto.so  \
    -DOPENSSL_SSL_LIBRARY=/usr/lib64/openssl3/libssl.so \
    && cmake --build . --parallel 8 && cmake --install . && rm -rf *
	
FROM build0 AS qtbuilder
COPY --from=qtbuilderbase ${QT_DIR} ${QT_DIR}

# https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
COPY appimagetool-${BUILD_ARCH}.AppImage /usr/local/bin/appimagetool
RUN chmod +x /usr/local/bin/appimagetool

# https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
COPY linuxdeploy-${BUILD_ARCH}.AppImage /usr/local/bin/linuxdeploy
RUN chmod +x /usr/local/bin/linuxdeploy

# https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
COPY linuxdeploy-plugin-qt-${BUILD_ARCH}.AppImage /usr/local/bin/linuxdeploy-plugin-qt
RUN chmod +x /usr/local/bin/linuxdeploy-plugin-qt

RUN echo "export PATH=$QT_DIR/bin:$PATH" >> /etc/profile.d/Qt${QT_VERSION}.sh && \
    echo "export LD_LIBRARY_PATH=$QT_DIR/lib:$QT_DIR/lib64:$LD_LIBRARY_PATH" >> /etc/profile.d/Qt${QT_VERSION}.sh && \
	echo "export APPIMAGE_EXTRACT_AND_RUN=1"  >> /etc/profile.d/appimage-in-docker.sh

WORKDIR /root
CMD ["bash"]

