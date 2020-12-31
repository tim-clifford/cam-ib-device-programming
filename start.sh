MOUNT_PATH=mnt
mkdir -p $MOUNT_PATH
if ! mount | grep -q NODE_F746ZG; then
	sudo mount -o umask=002,gid=$(id -u $(whoami)),uid=$(id -g $(whoami)) $(find /dev/disk/by-id/ -name *MBED*) $MOUNT_PATH 
fi
mbed compile -m NUCLEO_F746ZG -t GCC_ARM --flash
cp BUILD/NUCLEO_F746ZG/GCC_ARM/mbed-os-example-blinky.bin $MOUNT_PATH
