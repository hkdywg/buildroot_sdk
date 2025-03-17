# 项目简介

buildroot_sdk 是一个基于buildroot的编译平台，用来编译kernel/uboot/rootfs，同时生成可烧录的image固件。

# SDK目录结构

```
├── build               // 编译目录，存放编译脚本
├── build.sh            // 一键编译脚本
├── buildroot-2021.05   // buildroot 开源工具
├── build_out           // 执行一次完整编译后，存放编译输出文件及image
├── kernel              // 开源 linux 内核
├── device              // 存放board相关配置及差异化文件
├── ramdisk             // 存放最小文件系统的 prebuilt 目录
└── u-boot-2021.10      // 开源 uboot 代码
```

# 快速开始

## 安装编译依赖工具包

```bash
sudo apt install -y pkg-config build-essential ninja-build automake autoconf libtool wget curl git gcc libssl-dev bc slib squashfs-tools android-sdk-libsparse-utils jq python3-distutils scons parallel tree python3-dev python3-pip device-tree-compiler ssh cpio fakeroot libncurses5 flex bison libncurses5-dev genext2fs rsync unzip dosfstools mtools tcl openssh-client cmake expect
```

## 获取SDK

```bash
git clone https://github.com/hkdywg/buildroot_sdk.git --depth=1
```

## 一键编译

执行一键编译脚本 `build.sh`：

```bash
cd buildroot_sdk/
./build.sh
```

会看到编译脚本的使用方法提示:

```
Usage:
./build.sh          - Show this menu
./build.sh lunch        - Select a project to update
./build.sh [project]    - build [project] directly, support project as  follows:
rcar_e3_control_box
```

最下边列出的是目前支持的board列表。如提示中所示，有两种方法编译

第一种方法是执行 `./build.sh lunch` 调出交互菜单，选择要编译的版本序号，回车：

```
Select a target project
1. rcar_e3_control_box
Which would you select:
```

第二种方法是脚本后面带上目标板的名字 ，如

```
./build.sh rcar_e3_control_box
```
