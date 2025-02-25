
#
# some basic check
#
ifneq ($(origin TOP_DIR),environment)
	$(error Please source setup.sh first)
endif

ifneq ($(wildcard ${TOP_DIR}/build/Makefile), $(abspath Makefile))
	$(error $${TOP_DIR}/build/Makefile is not current Makefile)
endif

# Verbose flag
ifeq (${BUILD_VERBOSE},0)
	Q:=@
else
	Q:=
endif
export Q

define print_target
	@printf '\033[1;36;40m [TARGET] $@ \033[0m\n'
endef


########################################################################
# kernel targets
########################################################################
# configure kernel defconfig
ifeq ($(CONFIG_BUILD_FOR_DEBUG), y)
KERNEL_COFNIG_NAME := kernel_debug_defconfig
else
KERNEL_COFNIG_NAME := kernel_release_defconfig
endif

${KERNEL_BUILD_PATH}:
	${Q}mkdir -p $@

define copy_Image_action
	${Q}mkdir -p ${KERNEL_BUILD_PATH}/elf
	${Q}cp ${KERNEL_BUILD_PATH}/arch/${KERNEL_ARCH}/boot/Image ${KERNEL_BUILD_PATH}/elf/
	${Q}cp ${KERNEL_BUILD_PATH}/vmlinux ${KERNEL_BUILD_PATH}/elf/
endef

define copy_ko_action
	${Q}mkdir -p ${KERNEL_BUILD_PATH}/ko
	${Q}find ${1} -name '*.ko' -exec cp -f {} ${KERNEL_BUILD_PATH}/ko/ \;
endef

kernel%: export INSTALL_MOD_PATH=${KERNEL_BUILD_PATH}/modules
kernel%: export ARCH=${KERNEL_ARCH}

$(warning ${KERNEL_ARCH})

KERNEL_OUTPUT_CONFIG_PATH := ${KERNEL_BUILD_PATH}/.config
KERNEL_DEFAULT_COFNIG_PATH := ${TOP_DIR}/device/${BUILD_PROJECT}/config/${KERNEL_COFNIG_NAME}

${KERNEL_OUTPUT_CONFIG_PATH}: ${KERNEL_DEFAULT_COFNIG_PATH}
	$(call print_target})
	${Q}echo "Kenel's defconfig is update. Use it"
	${Q}mkdir -p $(dir ${KERNEL_OUTPUT_CONFIG_PATH})
	${Q}cmp -s ${KERNEL_DEFAULT_COFNIG_PATH} ${KERNEL_OUTPUT_CONFIG_PATH} || \
		${Q}cp -vb ${KERNEL_DEFAULT_COFNIG_PATH} ${KERNEL_OUTPUT_CONFIG_PATH}

kernel-menuconfig: ${KERNEL_OUTPUT_CONFIG_PATH}
	$(call print_target)
	${Q}$(MAKE) -j${COREN} -C ${KERNEL_PATH} O=${KERNEL_BUILD_PATH} menuconfig

kernel-setconfig: ${KERNEL_OUTPUT_CONFIG_PATH}
	$(call print_target)
	${Q}$(MAKE) -j${COREN} -C ${KERNEL_PATH} O=${KERNEL_BUILD_PATH} setconfig 'SCRIPT_ARG=${SCRIPT_ARG}'
	${Q}$(MAKE) -j${COREN} -C ${KERNEL_PATH} O=${KERNEL_BUILD_PATH} savedefconfig

kernel-build: ${KERNEL_OUTPUT_CONFIG_PATH}
	$(call print_target)
	${Q}$(MAKE) -j${COREN} -C ${KERNEL_PATH} O=${KERNEL_BUILD_PATH} Image modules
	${Q}$(MAKE) -j${COREN} -C ${KERNEL_PATH} O=${KERNEL_BUILD_PATH} modules_install

kernel: kernel-build
	$(call print_target)
	$(call copy_Image_action)
	$(call copy_ko_action, ${INSTALL_MOD_PATH})


########################################################################
# rootfs targets
########################################################################

${ROOTFS_BUILD_PATH}:
	${Q}mkdir -p $@

br-rootfs-prepare:export INSTALL_MOD_PATH=${KERNEL_BUILD_PATH}/modules
br-rootfs-prepare:export BR_OVERLAY_PATH=${TOP_DIR}/buildroot-2021.05/board/overlay
br-rootfs-prepare:
	$(call print_target)
	#${Q}ln -s ${TOP_DIR}/device/${BUILD_PROJECT}/overlay ${BR_OVERLAY_PATH}
	${Q}rm -rf ${BR_OVERLAY_PATH}/*
	# copy system ko file 
	${Q}cp -arf ${INSTALL_MOD_PATH}/lib ${BR_OVERLAY_PATH}/
	# copy device overlay files
	${Q}cp -arf ${TOP_DIR}/device/common/overlay/* ${BR_OVERLAY_PATH}/
	${Q}cp -arf ${TOP_DIR}/device/${BUILD_PROJECT}/overlay_file/* ${BR_OVERLAY_PATH}/
	# strip
	${Q}find ${BR_OVERLAY_PATH} -name "*.ko" -type f -printf 'striping %p\n' -exec ${CROSS_COMPILE}strip --strip-unneeded {} \;
	${Q}find ${BR_OVERLAY_PATH} -name "*.so*" -type f -printf 'striping %p\n' -exec ${CROSS_COMPILE}strip --strip-all {} \;
	${Q}find ${BR_OVERLAY_PATH} -executable -type f ! -name "*.sh" ! -path "*etc*" -exec ${CROSS_COMPILE}strip --strip-all {} \;
	
 
br-rootfs-build:export TARGET_OUTPUT_DIR=${BR_BUILD_PATH}
br-rootfs-build:export BR_DEFCONFIG=${BUILD_PROJECT}_br_defconfig
br-rootfs-build:
	$(call print_target)
	${Q}$(MAKE) -C ${ROOTFS_PATH} ${BR_DEFCONFIG} BR2_TOOLCHAIN_EXTERNAL_PATH=${CROSS_COMPILE_PATH} O=${BR_BUILD_PATH}
	${Q}${ROOTFS_PATH}/utils/brmake -j${COREN} -C ${ROOTFS_PATH} O=${BR_BUILD_PATH}

rootfs-build: br-rootfs-build

rootfs: br-rootfs-prepare rootfs-build
	$(call print_target)




