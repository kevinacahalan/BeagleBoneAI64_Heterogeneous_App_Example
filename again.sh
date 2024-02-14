	#sudo cp test.elf /lib/firmware/
	sudo echo stop > /sys/class/remoteproc/remoteproc18/state
	#sudo echo test.elf > /sys/class/remoteproc/remoteproc18/firmware
	sudo echo start > /sys/class/remoteproc/remoteproc18/state
	sudo cat /sys/kernel/debug/remoteproc/remoteproc18/trace0

