
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
# uboot targets
########################################################################

# configure kernel defconfig
ifeq ($(CONFIG_BUILD_FOR_DEBUG), y)
UBOOT_COFNIG_NAME := uboot_debug_defconfig
else
UBOOT_COFNIG_NAME := uboot_release_defconfig
endif
UBOOT_OUT_CONFIG_PATH := ${UBOOT_BUILD_PATH}/.config
UBOOT_DEFAULT_COFNIG_PATH := ${TOP_DIR}/device/${BUILD_PROJECT}/config/${UBOOT_COFNIG_NAME}

define uboot_release_action
	${Q}sed -i "s/CONFIG_BOOTDELAY=1/CONFIG_BOOTDELAY=-2/g" ${UBOOT_OUT_CONFIG_PATH}
endef

${UBOOT_OUT_CONFIG_PATH}: ${UBOOT_DEFAULT_COFNIG_PATH}
	$(call print_target})
	${Q}echo "uboot's defconfig is update. Use it"
	${Q}mkdir -p $(dir ${UBOOT_OUT_CONFIG_PATH})
	${Q}cmp -s ${UBOOT_DEFAULT_COFNIG_PATH} ${UBOOT_OUT_CONFIG_PATH} || \
		${Q}cp -vb ${UBOOT_DEFAULT_COFNIG_PATH} ${UBOOT_OUT_CONFIG_PATH}
ifeq ($(CONFIG_BUILD_FOR_DEBUG), )
	$(call uboot_release_action)
endif

u-boo%: export KBUILD_OUTPUT=${UBOOT_BUILD_PATH}

u-boot-menuconfig: ${UBOOT_OUT_CONFIG_PATH}
	$(call print_target)
	${Q}$(MAKE) -j${COREN} -C ${UBOOT_PATH} menuconfig
	${Q}$(MAKE) -j${COREN} -C ${UBOOT_PATH} savedefconfig

u-boot:${IMAGE_OUTPUT_PATH}/rawimages
u-boot:${UBOOT_OUT_CONFIG_PATH}
u-boot:
	$(call print_target)
	${Q}$(MAKE) -j${COREN} -C ${UBOOT_PATH} olddefconfig
ifeq ($(KERNEL_ARCH), arm64)
	${Q}$(MAKE) -j${COREN} -C ${UBOOT_PATH} ARCH=arm all
endif
	${Q}cp ${UBOOT_BUILD_PATH}/u-boot.bin ${IMAGE_OUTPUT_PATH}/rawimages/

u-boot-clean:
	$(call print_target)
	${Q}${MAKE} -j${COREN} -C ${UBOOT_PATH} distclean



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
	${Q}mkdir -p ${RAMDISK_OUTPUT_PATH}/workspace
	${Q}cp ${KERNEL_BUILD_PATH}/arch/${KERNEL_ARCH}/boot/Image ${RAMDISK_OUTPUT_PATH}/workspace/
	${Q}cp ${KERNEL_BUILD_PATH}/vmlinux ${KERNEL_BUILD_PATH}/elf/
endef

define copy_ko_action
	${Q}mkdir -p ${KERNEL_BUILD_PATH}/ko
	${Q}find ${1} -name '*.ko' -exec cp -f {} ${KERNEL_BUILD_PATH}/ko/ \;
endef

kerne%: export INSTALL_MOD_PATH=${KERNEL_BUILD_PATH}/modules
kerne%: export ARCH=${KERNEL_ARCH}


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

kernel-dts:
	$(call print_target)
	${Q}$(MAKE) -j${COREN} -C ${KERNEL_PATH} O=${KERNEL_BUILD_PATH} dtbs

kernel:${RAMDISK_OUTPUT_PATH}/workspace
kernel: kernel-build kernel-dts 
	$(call print_target)
	$(call copy_Image_action)
	$(call copy_ko_action, ${INSTALL_MOD_PATH})
	${Q}cp ${KERNEL_BUILD_PATH}/arch/${KERNEL_ARCH}/boot/dts/${BRAND}/*.dtb  ${RAMDISK_OUTPUT_PATH}/workspace

kernel-clean:
	$(call print_target)
	${Q}${MAKE} -j${COREN} -C ${KERNEL_PATH} distclean
	${Q}$(if $(wildcard ${KERNEL_BUILD_PATH}), rm -rf ${KERNEL_BUILD_PATH},)
	${Q}rm -f ${KERNEL_BUILD_PATH}/elf/vmlinux
	${Q}$(if $(wildcard ${INSTALL_MODE_PATH}), rm -rf ${INSTALL_MODE_PATH},)

########################################################################
# kernel dtb
########################################################################
DTB_HOST_CC := gcc
HOST_DTC := dtc

DTB_CFLAGS := -E -Wp,-MMD,

DTC_FLAGS := 0 -Wno-unit_address_vs_reg -Wno-unit_address_format \
			 -Wno-avoid_unnecessary_addr_size -Wno-alias_paths -Wno-graph_child_address \
			 -Wno-simple_bus_reg -Wno-unique_unit_address -Wno-pci_device_reg

DTC_DEF := -undef -D__DTS__ -x assembler-with-cpp

DTS_SOURCES := $(wildcard $(KERNEL_DTB_PATH)/*.dts)                                                                    
DTB_D_PRE_TMP := $(patsubst %.dts,%.dtb.d.pre.tmp,$(notdir $(DTS_SOURCES)))
DTB_DTS_TMP := $(patsubst %.dts,%.dtb.dts.tmp,$(notdir $(DTS_SOURCES)))
DTB_D_DTC_TMP := $(patsubst %.dts,%.dtb.d.dtc.tmp,$(notdir $(DTS_SOURCES)))
DTB_D_TMP := $(patsubst %.dts,%.dtb.dts.tmp,$(notdir $(DTS_SOURCES)))
DTB_TARGET := $(patsubst %.dts,%.dtb,$(notdir $(DTS_SOURCES)))

kernel_dtb:
	@mkdir -p $(KERNEL_DTB_BUILD_PATH)
	@$(DTB_HOST_CC) -E $(DTB_CFLAGS)$(KERNEL_DTB_BUILD_PATH)/$(DTB_D_PRE_TMP) -nostdinc -I$(KERNEL_DTB_PATH)/include-prefixes \
		-I$(KERNEL_DTB_PATH)/include $(DTC_DEF) -o $(KERNEL_DTB_BUILD_PATH)/$(DTB_D_TMP) $(DTS_SOURCES)
	@$(HOST_DTC) -O dtb -o $(KERNEL_DTB_BUILD_PATH)/$(DTB_TARGET) -b $(DTC_FLAGS) -d $(KERNEL_DTB_BUILD_PATH)/$(DTB_D_DTC_TMP) $(KERNEL_DTB_BUILD_PATH)/$(DTB_D_TMP)

clean:
	@rm -rf $(KERNEL_DTB_BUILD_PATH)/*


########################################################################
# boot targets
########################################################################

define gen_cpio
	cd ${RAMDISK_OUTPUT_PATH}/configs; \
	${PREBUILD_TOOLS_PATH}/gen_init_cpio ${RAMDISK_OUTPUT_PATH}/configs/$(1) > ${RAMDISK_OUTPUT_PATH}/workspace/ramfs.cpio
endef

define gen_boot
	cd ${RAMDISK_OUTPUT_PATH}/workspace; \
	cp ${TOP_DIR}/device/${BUILD_PROJECT}/multi.its ./multi.its; \
	${UBOOT_BUILD_PATH}/tools/mkimage -f ./multi.its -r ${RAMDISK_OUTPUT_PATH}/workspace/boot.emmc
endef


${RAMDISK_OUTPUT_PATH}/target:
	${Q}mkdir -p $@

${RAMDISK_OUTPUT_PATH}/workspace:
	${Q}mkdir -p $@

${RAMDISK_OUTPUT_PATH}/configs:
	${Q}mkdir -p $@

ramdisk: ${RAMDISK_OUTPUT_PATH}/target
ramdisk: ${RAMDISK_OUTPUT_PATH}/configs
ramdisk: ${RAMDISK_OUTPUT_PATH}/workspace
ramdisk:
	${Q}rm -rf ${RAMDISK_OUTPUT_PATH}/target/*
	${Q}rm -rf ${RAMDISK_OUTPUT_PATH}/workspace/*
	${Q}cp -rf ${RAMDISK_PATH}/init_ramfs/* ${RAMDISK_OUTPUT_PATH}/target/
	${Q}cp -rf ${RAMDISK_PATH}/configs/* ${RAMDISK_OUTPUT_PATH}/configs/
	${Q}cp -rf ${TOP_DIR}/device/${BUILD_PROJECT}/overlay/ramfs/*  ${RAMDISK_OUTPUT_PATH}/target/

boot: ramdisk kernel u-boot
	$(call print_target)
	$(call gen_cpio,onekernel_fixed_files.txt)
	${Q}gzip -9 -f -k ${RAMDISK_OUTPUT_PATH}/workspace/Image > ${RAMDISK_OUTPUT_PATH}/workspace/Image.gz
	${Q}gzip -9 -f -k ${RAMDISK_OUTPUT_PATH}/workspace/ramfs.cpio > ${RAMDISK_OUTPUT_PATH}/workspace/ramfs.cpio.gz
	$(call gen_boot)
	${Q}cp ${RAMDISK_OUTPUT_PATH}/workspace/boot.emmc ${IMAGE_OUTPUT_PATH}/rawimages

ramboot: kernel
	$(call print_target)
	$(call gen_cpio,ramboot_fixed_files.txt)
	${Q}gzip -9 -f -k ${RAMDISK_OUTPUT_PATH}/workspace/Image > ${RAMDISK_OUTPUT_PATH}/workspace/Image.gz
	${Q}gzip -9 -f -k ${RAMDISK_OUTPUT_PATH}/workspace/ramfs.cpio > ${RAMDISK_OUTPUT_PATH}/workspace/ramfs.cpio.gz
	$(call gen_boot)
	${Q}cp ${RAMDISK_OUTPUT_PATH}/workspace/boot.emmc ${IMAGE_OUTPUT_PATH}/rawimages


########################################################################
# rootfs targets
########################################################################

define raw2cimg
	${Q}python3 ${TOP_DIR}/build/tools/image_tool/raw2cimg.py ${IMAGE_OUTPUT_PATH}/rawimages/${1} ${IMAGE_OUTPUT_PATH} ${PARTITION_XML}
endef

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
	${Q}cp -arf ${TOP_DIR}/device/${BUILD_PROJECT}/overlay/rootfs/* ${BR_OVERLAY_PATH}/
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

${IMAGE_OUTPUT_PATH}/rawimages:
	${Q}mkdir -p $@

br-rootfs-pack:${IMAGE_OUTPUT_PATH}/rawimages
	$(call print_target)
	${Q}cp  ${BR_BUILD_PATH}/images/rootfs.ext4 ${IMAGE_OUTPUT_PATH}/rawimages/rootfs.emmc
	$(call raw2cimg ,rootfs.emmc)


rootfs: br-rootfs-prepare br-rootfs-build br-rootfs-pack
	$(call print_target)

rootfs-clean:
	$(call print_target)
	${Q}rm -rf ${BR_BUILD_PATH}/target/* 


########################################################################
# system targets
########################################################################

define pack_image
${Q}${BUILD_PATH}/tools/prebuild/make_ext4fs -l ${3} -L $(shell echo ${1} | tr '[:lower:]' '[:upper:]') ${IMAGE_OUTPUT_PATH}/rawimages/${1}.emmc ${2}
endef

${BUILD_OUT_PATH}/system:
	${Q}mkdir -p $@

${IMAGE_OUTPUT_PATH}/rawimages/system.emmc: ${BUILD_OUT_PATH}/system
	$(call pack_image,system,${BUILD_OUT_PATH}/system,40M)

system:${IMAGE_OUTPUT_PATH}/rawimages/system.emmc
system:
	$(call print_target)
	$(call raw2cimg ,system.emmc)






