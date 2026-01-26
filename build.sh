#!/bin/bash
# Top level script for build system
#Usage: 
#./build.sh              - Show this menu
#./build.sh lunch        - Select a project to update
#./build.sh [project]    - build [project] directly, support project as  follows:


TOOLCHAIN_URL="https://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/aarch64-linux-gnu/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz"
BUILD_PROJECT=
BUILD_PROJECT_CONFIG=
PROJECT_ARRAY=

TOP_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")"  &> /dev/null && pwd)
cd ${TOP_DIR}
COREN=$(cat /proc/cpuinfo | grep 'processor' | wc -l)
TOOLCHAIN_PATH=${TOP_DIR}/tool_chain

export TOP_DIR COREN

function print_info()
{
	printf "\e[1;32m%s\e[0m\n" "$1"
}

function print_error()
{
	printf "\e[1;31mError: %s\e[0m\n" "$1"
}

function build_usage()
{
	echo "Usage: "
	echo ""$0"          	- Show this menu"
	echo ""$0" lunch        - Select a project to update"
	echo ""$0" [project]    - build [project] directly, support project as  follows:"

	for project in "${PROJECT_ARRAY}"; do
		print_info "$project"
	done
}

function get_toolchain()
{
	#toolchain_file=${TOOLCHAIN_URL##*/}
	#TOOLCHAIN_PATH=${TOP_DIR}/tool_chain/$(basename "${toolchain_file}" .tar.xz)
	if [ ! -d ${TOOLCHAIN_PATH} ]; then
		print_info "Toolchain does not exist, download it now ..."
		mkdir -p ${TOOLCHAIN_PATH}
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


function get_available_project()
{
    PROJECT_ARRAY=$(find device -mindepth 1 -maxdepth 1 -not -path "device/common" -type d -print ! -name "." | awk -F/ '{print $NF}' | sort -t '-' -k2,2)
    #print_info "${PROJECT_ARRAY}"
    local len
    len=$(echo $PROJECT_ARRAY | tr -cd ' ' | wc -c)
    PROJECT_ARRAY_LEN=$(($len+1))
	if [ ${PROJECT_ARRAY_LEN} -eq 0 ];then
		print_error "No available config board"
		exit 1
	fi
}

function choose_project()
{
    echo "Select a target project"

    echo ${PROJECT_ARRAY} | xargs -n 1 | sed "=" | sed "N;s/\n/. /"

    local index
    read -p "Which would you select: " index

    if [ -z $index ]; then
    print_err "Nothing select"
    exit 0
    fi

    if [[ -n $index && $index =~ ^[0-9]+$ && $index -ge 1 && $index -le $PROJECT_ARRAY_LEN ]]; then
    #set -- $PROJECT_ARRAY
    BUILD_PROJECT=$(echo $PROJECT_ARRAY | awk -v field="$index" '{print $field}')
    else
    print_err "Invalid input!"
    exit 1
    fi
}

function prepare_env()
{
	BUILD_PROJECT_CONFIG=device/${BUILD_PROJECT}/config/project_config
	if [ ! -f ${BUILD_PROJECT_CONFIG} ]; then
		print_error "${BUILD_PROJECT_CONFIG} not found!"
		exit 1
	fi

	source ${BUILD_PROJECT_CONFIG}
	source build/setup.sh
	#source build/setup.sh > /dev/null 2>&1
}

function project_build() 
{
	build_all
	if [ $? -eq 0 ];then
		print_info "Build project ${BUILD_PROJECT} success!"
	else
		print_error "Build project  ${BUILD_PROJECT} failed!"
		exit 1
	fi
}

function build_spec_subtarget()
{
	local target
	local support_target=("uboot" "kernel"  "kernel_dtb" "rootfs" "boot" "gpt" "system")
	for target in "${support_target[@]}";  do
		if [ "$target" = "$1" ]; then		
			prepare_env
			build_$1 && exit $? 
		fi
	done
}


get_toolchain
get_available_project
if [  $# -eq 1 -o $# -eq 2 ]; then
    if [ "$1" = "lunch" ]; then
    choose_project || exit 0
    else
    target=$1
    found=0
    #set -- $PROJECT_ARRAY
    for word in $PROJECT_ARRAY; do
        if [ "$word" = "$target" ]; then
        BUILD_PROJECT=$target
        found=1
        break
        fi
    done
    if [ $found -eq 0 ]; then
        print_error "input project is invalid, not support yet!"
        exit 1
    fi
    fi
else
    build_usage && exit 0
fi
prepare_env
if [ $# -eq 2 ]; then
	build_spec_subtarget $2
	exit
fi
project_build

