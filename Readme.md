* Clone with\
`git clone --recurse-submodules git@github.com:vacore/1inch.git`

* To build and run the development environment container\
`./run.sh`

* In the docker container under `~/repo/workspace/nuttx-os` run either of the two (anytime, makes distclean):\
`./tools/configure.sh -E -l -a ../nuttx-apps sim:1inch`\
`./tools/configure.sh -E -l -a ../nuttx-apps stm32f746g-disco:1inch`

* Work on the current config (optionally)\
`make menuconfig`

* Build after the configuration\
`make`

* Update the source config if changed (for committing)\
`make savedefconfig`\
`cp defconfig ../configs/sim/1inch/`\
`#cp defconfig ../configs/stm32f746g-disco/1inch/`
