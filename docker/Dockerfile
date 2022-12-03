FROM ubuntu:20.04

SHELL ["/bin/bash", "-c"]

ARG DEBIAN_FRONTEND="noninteractive" TZ
ENV TZ="$TZ"
RUN apt-get --assume-yes --ignore-missing update &&  \
  apt-get --no-install-recommends --yes install  \
    apt-utils               \
    automake                \
    binutils-arm-none-eabi  \
    bison                   \
    ccache                  \
    entr                    \
    file                    \
    flex                    \
    gcc                     \
    gcc-arm-none-eabi       \
    genromfs                \
    git-core                \
    gperf                   \
    kconfig-frontends       \
    less                    \
    libncurses5-dev         \
    libz-dev                \
    locales                 \
    make                    \
    openocd                 \
    picocom                 \
    pkg-config              \
    python3                 \
    python3-pip             \
    sudo                    \
    xxd                    &&  \
  dpkg-reconfigure locales &&  \
  locale-gen en_US.UTF-8   &&  \
  update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

RUN pip install pyelftools cxxfilt

RUN for c in cc gcc arm-none-eabi-gcc ;  \
  do ln -s `which ccache` /usr/local/bin/$c; done

ARG gid uid group user
RUN groupadd $group -g $gid               &&  \
  useradd -m $user -g $group -s /bin/bash &&  \
  adduser $user sudo                      &&  \
  usermod -u $uid $user                   &&  \
  echo "$user:$user" | chpasswd

ARG user group
USER $user:$group

RUN {                                       \
  echo 'umask 0022'                      ;  \
  echo 'export LANG=en_US.UTF-8'         ;  \
  echo 'export LC_ALL=en_US.UTF-8'       ;  \
  echo 'export MAKEFLAGS=--jobs=$(nproc)';  \
} >> $HOME/.bashrc

RUN ccache -o cache_dir=$HOME/repo/ccache

WORKDIR /home/$user/repo/workspace/nuttx-os
ENTRYPOINT ["/bin/bash"]
