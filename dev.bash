#!/usr/bin/env bash
set -eu

docker build -t proftest .
docker run \
  -it \
  --rm \
  --privileged \
  -v /lib/modules:/lib/modules:ro \
  -v /etc/localtime:/etc/localtime:ro \
  -v `pwd`:/proftest \
  --pid=host \
  proftest
