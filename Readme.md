# Instructions
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

# Code notes
	2 queues:
		1. To send random numbers downstream from RNG thread to the matrix multiplication thread
		2. To send products from the multiplication thread to the SPI sending thread
	3 tasks (in descending priority order):
		1. Generates random numbers for two matrices, puts the produced set on queue.
		2. Takes the RNG-sets from the queue(1), produces the multiplication, puts it on queue(2)
		3. Takes the product from the queue(2), sends it to the SPI
 */
