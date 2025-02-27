#!/bin/bash

# partition generation:
# pack_boot -> boot.emmc / boot.spinand (kernel+dtb)
# pack_rootfs -> rootfs.emmc / rootfs.spinand ( rootfs )
# pack_system -> system.emmc / system.spinand (3rd/sdk shared libraries,  spinand-ubifs/ emmc-ext4)
# pack_gpt -> gpt.img (emmc only)

function path_remove()
{
	local IFS=':'						
	local newpath dir 					
	local path_variable=${2:-PATH}		

	for dir in ${!path_variable};  do
		if [ "$dir" != "$1" ]; then	
			newpath=${newpath:+$newpath:}$dir
		fi
	done

	export $path_variable="$newpath"
}

function path_prepend()
{
	path_remove $1 $2
	local path_variable=${2:-PATH}
	export $path_variable="$1${!path_variable:+:${!path_variable}}"
}

function path_pend()
{
	path_remove $1 $2
	local path_variable=${2:-PATH}
	export $path_variable="${!path_variable:+${!path_variable}:}$1"
}

function build_uboot()
{
	cd ${BUILD_PATH} || return
	make u-boot || return $?
}

function build_kernel()
{
	cd ${BUILD_PATH} || return
	make kernel || return $?
}

function build_rootfs()
{
	cd ${BUILD_PATH} || return
	make rootfs || return $?
}

function build_boot()
{
	cd ${BUILD_PATH} || return
	make boot || return $?
}

function build_gpt()
{
	mkdir -p "${BUILD_OUT_PATH}"/image/rawimages
	python3 ${BUILD_PATH}/tools/image_tool/gen_gpt.py ${TOP_DIR}/device/${BUILD_PROJECT}/config/partition_emmc.xml ${BUILD_OUT_PATH}/image/rawimages/ 
}

function build_img()
{
	python3 ${BUILD_PATH}/tools/image_tool/mergeimg.py ${TOP_DIR}/device/${BUILD_PROJECT}/config/partition_emmc.xml ${BUILD_OUT_PATH}/image/rawimages/ 
}

function build_system()
{
	cd ${BUILD_PATH} || return
	make system || return $?
}

function build_all()
{
	# build bsp
	build_uboot || return $?
	build_kernel || return $?
	build_rootfs || return $?
	build_boot || return $?
	build_gpt || return $?
	build_system || return $?
	build_img || return $?
}

function clean_all()
{
#	clean_uboot
# 	clean_kernel
	echo "clean"
}

