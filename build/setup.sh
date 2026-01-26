#!/bin/bash

source "$TOP_DIR/build/build_core.sh"

function __build_default_env()
{
	BRAND=${BRAND:-renesas}
	DEBUG=${DEBUG:-0}
	RELEASE_VERSION=${RELEASE_VERSION:-0}
	BUILD_VERBOSE=${BUILD_VERBOSE:-1}
}

function sdk_version()
{
	if [ -n "$1" ]; then
		SDK_VERSION="$1"
	fi

	if [ "$SDK_VERSION" = 64bit ]; then
		CROSS_COMPILE=${CROSS_COMPILE_64}
		CROSS_COMPILE_PATH=${CROSS_COMPILE_PATH_64}
		KERNEL_ARCH=arm64
	elif [ "$SDK_VERSION" = 32bit ]; then
		CROSS_COMPILE=${CROSS_COMPILE_32}
		CROSS_COMPILE_PATH=${CROSS_COMPILE_PATH_32}
		KERNEL_ARCH=arm
	fi
}

function setup_build_env()
{
	__build_default_env

	# build core path
	BUILD_PATH="$TOP_DIR"/build
	# top buildout path
	BUILD_OUT_PATH=${TOP_DIR}/build_out/"$BUILD_PROJECT"

	# output folder path
	INSTALL_PATH="${BUILD_OUT_PATH}"/install
	UBOOT_BUILD_PATH="${BUILD_OUT_PATH}"/uboot
	KERNEL_BUILD_PATH="${BUILD_OUT_PATH}"/kernel
    KERNEL_DTB_BUILD_PATH="${BUILD_OUT_PATH}"/kernel_dtb
	ROOTFS_BUILD_PATH="${BUILD_OUT_PATH}"/rootfs
	IMAGE_OUTPUT_PATH="${BUILD_OUT_PATH}"/image
	RAMDISK_OUTPUT_PATH="${BUILD_OUT_PATH}"/ramdisk

	#  source file folders
	UBOOT_PATH="${TOP_DIR}"/uboot
	KERNEL_PATH="${TOP_DIR}"/kernel
	KERNEL_DTB_PATH="${TOP_DIR}"/kernel_dtb
	KERNEL_DRIVER_PATH="${TOP_DIR}"/linux_driver
	RAMDISK_PATH="${TOP_DIR}"/ramdisk
	ROOTFS_PATH="${TOP_DIR}"/buildroot-2021.05
	APPS_PATH="${TOP_DIR}"/apps
	PREBUILD_TOOLS_PATH="${BUILD_PATH}"/tools/prebuild

	export BRAND DEBUG RELEASE_VERSION BUILD_VERBOSE BUILD_PROJECT
	export BUILD_PATH BUILD_OUT_PATH UBOOT_BUILD_PATH KERNEL_BUILD_PATH KERNEL_DTB_BUILD_PATH ROOTFS_BUILD_PATH IMAGE_OUTPUT_PATH RAMDISK_OUTPUT_PATH
	export UBOOT_PATH KERNEL_PATH KERNEL_DTB_PATH KERNEL_DRIVER_PATH RAMDISK_PATH ROOTFS_PATH APPS_PATH PREBUILD_TOOLS_PATH

	# buildroot config
	export BR_BUILD_PATH="${BUILD_OUT_PATH}"/buildroot
	export BR_INSTALL_PATH=${BR_BUILD_PATH}/tmp_rootfs

	# toolchain path
	CROSS_COMPILE_64=aarch64-linux-gnu-
	CROSS_COMPILE_32=arm-linux-gnueabihf-
	CROSS_COMPILE_PATH_64=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu
	CROSS_COMPILE_PATH_32=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf

	sdk_version

	path_prepend "$CROSS_COMPILE_PATH"/bin
	export CROSS_COMPILE_PATH CROSS_COMPILE KERNEL_ARCH

	# configure partition table
	PARTITION_XML="${TOP_DIR}"/device/${BUILD_PROJECT}/config/partition_emmc.xml
	if [ ! -e ${PARTITION_XML} ];then
		print_error "${PARTITION_XML} does not exist!"
		return 1
	fi
	export  PARTITION_XML
}


setup_build_env

