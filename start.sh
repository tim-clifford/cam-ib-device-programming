MOUNT_PATH=mnt
mkdir -p $MOUNT_PATH
if ! mount | grep -q cam-ib-device-programming; then
	sudo -A mount -o umask=002,gid=$(id -u $(whoami)),uid=$(id -g $(whoami)) $(find /dev/disk/by-id/ -name *MBED*) $MOUNT_PATH 
fi
if mbed compile -m NUCLEO_F746ZG -t GCC_ARM --flash --sterm; then
	sudo -A cat `mbedls|grep NUCLEO_F746ZG|awk -F"|" '{print $5}'` 
fi
