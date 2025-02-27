#/bin/bash

if [ "$#" -ne "1" ]; then
	echo "usage:sudo ./check_image.sh IMAGE_PATH"
	echo ""
	echo " 	The scriptis used to create a loop device,"
	echo "	and mapping partition to loop device, the partition info is from image(include gpt)"
	echo " 	You will view and check the contents of image"
	echo ""
	echo "Note: The image is include gpt partition, not raw ext4 image!"
	exit
fi

IMAGE_FILE=$1

# step 1, create the loop device
loop_device=$(sudo losetup -f --show ${IMAGE_FILE})
echo ${loop_device}

# step 2, create image partition map
par_info=$(sudo kpartx -av ${loop_device})
par_num=$(echo ${par_info} | grep -o "add map" | wc -l)

# step 3, creat and mount local tmp path
for i in $(seq 1 ${par_num})
do
	mkdir -p tmp$i
	mount ${loop_device}p$i tmp$i
done

# step 4, umount and delete temp dir
for i in $(seq 1 ${par_num})
do
	sudo umount ./tmp$i
	rm -rf ./tmp$1
done

# step 5, delete img file and virtual device mapping
sudo kpartx -d ${loop_device}

# step 6, disconnect img file with loop deice 
sudo losetup -d ${loop_device}
