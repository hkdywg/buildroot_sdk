#/bin/bash

if [ "$#" -ne "1" ]; then
	echo "usage:sudo ./gen_sd_burn_image.sh OUTPUT_DIR"
	echo ""
	echo " 		The scriptis used to create a sdcard image with two partitions,"
	echo "		one is fat32 with 128M, the other is ext4 with 256M."
	echo " 		You can modify the capatities in this script as you wish!"
	echo ""
	echo "Note: Please  backup you sdcard fifle befor using this images!"

	exit
fi

vfat_cap=128M
vfat_label="boot"
ext4_cap=256M
ext4_label="rootfs"

output_dir=$1
echo ${output_dir}
pushd ${output_dir}

# generate a empty  image
image=burn_sd_`date +%Y%m%d-%H%M`.img
echo ${image}
dd if=/dev/zero of=./${image} bs=1M count=512

sudo fdisk ./${image}<< EOF
n
p
1

+${vfat_cap}
n
p
2

+${ext4_cap}
w
EOF

dev_name=`sudo losetup -f`
echo ${dev_name}
echo ""

sudo losetup ${dev_name} ./${image}
sudo partprobe ${dev_name}

sudo mkfs.vfat -F 32 -n ${vfat_label} ${dev_name}p1
sudo mkfs.ext4 -L ${ext4_label} ${dev_name}p2

# mount partition
rm ./tmp1 ./tmp2 -rf
mkdir tmp1  tmp2
sudo mount -t vfat  ${dev_name}p1 tmp1/
sudo mount -t ext4  ${dev_name}p2 tmp2/

# copy boot file and rootfs
sudo cp ./boot.bin ./tmp1/
sudo cp -raf rootfs/* ./tmp2/

sync

# umount
sudo umount tmp1 tmp2
sudo losetup -d ${dev_name}
rmdir tmp1 tmp2

# tar image
#tar czvf ${image}.tar.gz ${image}

echo "Generate iamge successful: ${iamge}"
echo ""

popd

