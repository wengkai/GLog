#!/bin/bash

this_dir=$(dirname "$(readlink -f "$0")")
echo $this_dir

cd $this_dir

cd .. && \
docker run --rm -v "$(pwd):/root/GLog" ghcr.io/llxx2013/qt693-builder:main "/root/GLog/linux-deploy/linux-deploy.sh"
