#!/bin/bash

source "$TOP_DIR/build/build_core.sh"

function __build_default_env()
{
	DEBUG=${DEBUG:-0}
	RELEASE_VERSION=${RELEASE_VERSION:-0}
	BUILD_VERBOSE=${BUILD_VERBOSE:-0}
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
	ROOTFS_BUILD_PATH="${BUILD_OUT_PATH}"/rootfs

	#  source file folders
	UBOOT_PATH="${TOP_DIR}"/uboot
	KERNEL_PATH="${TOP_DIR}"/kernel
	RAMDISK_PATH="${TOP_DIR}"/ramdisk
	ROOTFS_PATH="${TOP_DIR}"/buildroot
	APPS_PATH="${TOP_DIR}"/apps

	export DEBUG RELEASE_VERSION BUILD_VERBOSE BUILD_PROJECT
	export BUILD_PATH BUILD_OUT_PATH UBOOT_BUILD_PATH KERNEL_BUILD_PATH ROOTFS_BUILD_PATH
	export UBOOT_PATH KERNEL_PATH RAMDISK_PATH ROOTFS_PATH APPS_PATH 

	# toolchain path
	export CROSS_COMPILE_64="aarch64-linux-gnu-"
	export CROSS_COMPILE_32=arm-linux-gnueabihf-
	export KERNEL_ARCH=arm64
	CROSS_COMPILE_PATH_64=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu
	CROSS_COMPILE_PATH_32=${TOOLCHAIN_PATH}/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf
	path_prepend "$CROSS_COMPILE_PATH_64"/bin
	echo --------- $PATH	
	echo "toolchain path: ${TOOLCHAIN_PATH}"
}


setup_build_env

