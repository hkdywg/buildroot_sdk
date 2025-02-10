#!/bin/bash

TOOLCHAIN_URL="https://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/aarch64-linux-gnu/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz"

function print_info()
{
	printf "\e[1;32m%s\e[0m\n" "$1"
}

function print_error()
{
	printf "\e[1;31mError: %s\e[0m\n" "$1"
}

function get_toolchain()
{
	toolchain_file=${TOOLCHAIN_URL##*/}
	toolchain_path=tool_chain/$(basename "${toolchain_file}" .tar.xz)
	if [ ! -d ${toolchain_path} ]; then
		print_info "Toolchain does not exist, download it now ..."
		mkdir -p ${toolchain_path}
		echo "toolchain_url: ${TOOLCHAIN_URL}"
		echo "toolchain_file: ${toolchain_file}"

		wget ${TOOLCHAIN_URL} -O ${toolchain_file}
		if [ $? -ne 0 ];then
			print_error "Failed to download ${toolchain_file} !"
			exit 1
		fi

		if [ ! -f ${toolchain_file} ];then
			print_error "${toolchain_file} not found!"
			exit 1
		fi
		
		print_info "Extracting ${toolchain_file} ... "
		tar -xf ${toolchain_file} -C ./tool_chain
		if  [ $? -ne 0 ];then
			print_error "Extract ${toolchain_file} failed!"
			exit 1
		fi

		[ -f ${toolchain_file} ] && rm -rf ${toolchain_file}
	fi
}


get_toolchain
