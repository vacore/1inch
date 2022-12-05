#!/bin/bash
CUR="$(readlink -f $(dirname "${BASH_SOURCE[0]}"))"

name=nuttx
user=developer
group=software

# Patch nuttx-os
patch --strip=1 --forward --reject-file=- --directory="$CUR/workspace/nuttx-os" <nuttx-os.patch

# Create symlinks for configs and apps
pushd "$CUR/workspace"
 ln -frs configs/sim/1inch nuttx-os/boards/sim/sim/sim/configs
 ln -frs configs/stm32f746g-disco/1inch nuttx-os/boards/arm/stm32f7/stm32f746g-disco/configs
 ln -svr myapps/1inch nuttx-apps
popd


DOCKER_BUILDKIT=1 docker build             \
    --build-arg uid="$(id -u)"             \
    --build-arg gid="$(id -g)"             \
    --build-arg user="$user"               \
    --build-arg group="$group"             \
    --build-arg TZ="$(cat /etc/timezone)"  \
    -t docker_nuttx_image "$CUR/docker"

docker run                                    \
    --name="$name"                            \
    --network=host                            \
    --hostname="$name"                        \
    --add-host="$name:127.0.0.1"              \
    -v ~/.gitconfig:/home/"$user"/.gitconfig  \
    -v ~/.ssh:/home/"$user"/.ssh              \
    -v "$CUR":/home/"$user"/repo              \
    --rm -ti docker_nuttx_image:latest
