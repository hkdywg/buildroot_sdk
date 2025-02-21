#!/bin/bash

function path_remove()
{
	local IFS=':'						# 设置内部字段分隔符为冒号(:)
	local newpath dir 					# 临时变量，用于遍历及存储
	local path_variable=${2:-PATH}		# 如果第二个参数没有传递，则默认为PATH

	# 遍历路径变量中的每个目录
	for dir in ${!path_variable};  do
		if [ "$dir" != "$1" ]; then		# 如果当前目录不等于要删除的目录
			newpath=${newpath:+$newpath:}$dir	#将当前目录添加到newpath中
		fi
	done
	# 设置PATH为修改后的路径
	export $path_variable="$newpath"
}

function path_prepend()
{
	path_remove $1 $2
	local path_variable=${2:-PATH}		# 如果第二个参数没有传递，则默认为PATH
	export $path_variable="$1${!path_variable:+:${!path_variable}}"
}

function path_pend()
{
	path_remove $1 $2
	local path_variable=${2:-PATH}		# 如果第二个参数没有传递，则默认为PATH
	export $path_variable="${!path_variable:+${!path_variable}:}$1"
}

# linux 内核编译
function build_kernel()
{
	cd ${BUILD_PATH} || return
	make kernel || return $?
}

# rootfs 编译
function build_rootfs()
{
	cd ${BUILD_PATH} || return
	make rootfs || return $?
}

# 所有子项编译入口
function build_all()
{
	# build bsp
#	build_uboot || return $?
	build_kernel || return $?
#	build_rootfs || return $?
}

function clean_all()
{
#	clean_uboot
# 	clean_kernel
	echo "clean"
}

