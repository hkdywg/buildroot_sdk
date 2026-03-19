# Buildroot SDK

<p align="center">
  <img src="https://img.shields.io/badge/Platform-R%20Car%20E3-blue?style=for-the-badge&logo=linux" alt="R-Car E3">
  <img src="https://img.shields.io/badge/Architecture-ARM64-red?style=for-the-badge" alt="ARM64">
  <img src="https://img.shields.io/badge/BuildSystem-Kbuild-green?style=for-the-badge" alt="Kbuild">
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="License">
  <img src="https://img.shields.io/badge/Buildroot-2021.05-orange?style=for-the-badge" alt="Buildroot">
</p>

<p align="center">
  <strong>面向嵌入式 Linux 的一站式 BSP 构建解决方案</strong>
</p>

---

## 目录

- [项目简介](#项目简介)
- [主要特性](#主要特性)
- [目录结构](#目录结构)
- [快速开始](#快速开始)
- [构建系统详解](#构建系统详解)
- [驱动开发](#驱动开发)
- [镜像输出](#镜像输出)
- [配置选项](#配置选项)
- [常见问题](#常见问题)
- [相关资源](#相关资源)

---

## 项目简介

Buildroot SDK 是一套完整的嵌入式 Linux BSP（Board Support Package）构建系统，基于 Buildroot 2021.05 构建框架，专注于为 Renesas R-Car E3 系列芯片提供从引导程序到根文件系统的全流程构建能力。

---

## 主要特性

| 特性 | 说明 |
|------|------|
| **一站式构建** | Kbuild 统一管理 U-Boot、Kernel、RootFS、驱动等所有组件 |
| **灵活分区** | 支持 eMMC 分区表自动生成与镜像打包 |
| **双架构支持** | 同时支持 ARM64 和 ARM32 架构构建 |
| **驱动模块** | DRM 显示、以太网 PHY、传感器、SerDes 等驱动 |
| **交叉编译** | 集成 Linaro GCC 7.4.1 工具链 |
| **QEMU 模拟** | 支持在 QEMU 虚拟机中运行和调试 |

---

## 目录结构

```
Buildroot_SDK/
├── build/                          # 构建系统核心
│   ├── Makefile                     # 顶层编译规则
│   ├── setup.sh                     # 环境配置脚本
│   ├── build_core.sh                # 核心构建函数库
│   └── tools/                       # 构建辅助工具
│       ├── image_tool/              # 镜像生成工具
│       │   ├── gen_gpt.py           # GPT 分区表生成
│       │   ├── mergeimg.py          # 镜像合并工具
│       │   ├── mk_part.py           # 分区创建工具
│       │   ├── cimg2raw.py          # 镜像格式转换
│       │   └── raw2cimg.py          # 镜像格式转换
│       ├── prebuild/                # 预编译工具
│       └── scripts/                 # 构建脚本
│
├── device/                          # 设备配置文件
│   └── rcar_e3_control_box/         # R-Car E3 控制盒配置
│       └── config/                  # 板级配置
│           ├── project_config       # 项目配置（SDK版本等）
│           ├── kernel_debug_defconfig
│           ├── kernel_release_defconfig
│           ├── uboot_debug_defconfig
│           ├── uboot_release_defconfig
│           └── partition_emmc.xml   # eMMC 分区表定义
│
├── uboot/                           # U-Boot 引导程序源码
├── kernel/                          # Linux 内核源码
├── kernel_dtb/                      # 设备树 Blob (.dtb)
│
├── linux_driver/                    # Linux 内核驱动示例
│   ├── drm_debug/                   # DRM 显示驱动调试
│   │   ├── drm_debug.c              # 基础调试驱动
│   │   ├── drm_debug_modeset.c      # 显示模式设置
│   │   ├── drm_debug_pattern.c      # 测试图案生成
│   │   ├── drm_debugfs.c            # DebugFS 接口
│   │   └── drm_debug_direct_show.c # 直接显示示例
│   ├── eth_phy/                     # 以太网 PHY 驱动框架
│   ├── ina_219/                     # INA219 电流传感器驱动
│   ├── lt9211/                      # LT9211 Serializer 驱动
│   ├── max96745/                    # MAX96745 Deserializer 驱动
│   └── rotary_encoder/              # 旋转编码器驱动
│
├── ramdisk/                         # RAM Disk 内存文件系统
├── buildroot-2021.05/               # Buildroot 根文件系统构建系统
│
├── build.sh                         # 顶层构建脚本（主入口）
├── build.sh-completion.bash        # Bash 自动补全脚本
│
└── README.md                        # 本文档
```

---

## 快速开始

### 1. 环境要求

| 项目 | 最低要求 |
|------|----------|
| 操作系统 | Ubuntu 18.04+ / Debian 9+ |
| 架构 | x86_64 |
| 磁盘空间 | ≥ 40GB |
| 内存 | ≥ 8GB |
| 网络 | 需联网下载工具链和源码 |

### 2. 安装依赖

```bash
# ===================== 基础开发工具 =====================
sudo apt-get update
sudo apt-get install -y qemu gcc make gdb git figlet

# ===================== 编译工具链 =====================
sudo apt-get install -y libncurses5-dev iasl wget
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev

# ===================== Python 环境 =====================
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y python2.7-dev python-dev python-serial

# ===================== 网络与系统工具 =====================
sudo apt-get install -y gobject-introspection
sudo apt-get install -y bridge-utils uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev

# ===================== 存储与镜像工具 =====================
sudo apt-get install -y kpartx libsdl2-dev libsdl1.2-dev
sudo apt-get install -y debootstrap bsdtar
sudo apt-get install -y libelf-dev gcc-multilib g++-multilib
sudo apt-get install -y libcap-ng-dev libmount-dev libselinux1-dev
sudo apt-get install -y liblzma-dev

# ===================== 图形界面支持 =====================
sudo apt-get install -y gtk+-2.0 glib-2.0 libglade2-dev

# ===================== 构建工具 =====================
sudo apt-get install -y ninja-build

# ===================== 64 位系统额外依赖 =====================
sudo apt-get install -y lib32z1 lib32z1-dev libc6:i386
```

### 3. 选择目标平台

使用交互式菜单选择目标板：

```bash
./build.sh lunch
```

当前支持的平台：

| 序号 | 平台名称 | 说明 |
|------|----------|------|
| 1 | rcar_e3_control_box | R-Car E3 控制盒 |

### 4. 构建完整镜像

```bash
# 编译所有组件（uboot → kernel → rootfs → boot → gpt → system → img）
./build.sh <project_name>

# 示例：构建 R-Car E3 控制盒镜像
./build.sh rcar_e3_control_box
```

---

## 构建系统详解

### 构建流程

```
┌─────────────────────────────────────────────────────────────────┐
│                        构建流程                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐  │
│  │  U-Boot  │───▶│  Kernel  │───▶│  RootFS  │───▶│   Boot   │  │
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘  │
│       │              │                                   │      │
│       ▼              ▼                                   ▼      │
│  ┌──────────┐    ┌──────────┐                      ┌──────────┐  │
│  │Kernel DTB│    │  Driver  │                      │   GPT    │  │
│  └──────────┘    └──────────┘                      └──────────┘  │
│                                                         │       │
│                                                         ▼       │
│                                                  ┌──────────┐   │
│                                                  │  System  │   │
│                                                  └──────────┘   │
│                                                         │       │
│                                                         ▼       │
│                                                  ┌──────────┐   │
│                                                  │   IMG    │   │
│                                                  └──────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 分步构建

支持单独构建特定组件：

| 命令 | 说明 | 输出 |
|------|------|------|
| `./build.sh <project> uboot` | 构建 U-Boot 引导程序 | `u-boot.bin` |
| `./build.sh <project> kernel` | 构建 Linux 内核 | `Image` |
| `./build.sh <project> kernel_dtb` | 编译设备树 | `*.dtb` |
| `./build.sh <project> kernel_driver` | 构建内核驱动模块 | `*.ko` |
| `./build.sh <project> rootfs` | 构建根文件系统 | `rootfs.*` |
| `./build.sh <project> boot` | 打包启动镜像 | `boot.emmc` |
| `./build.sh <project> gpt` | 生成 GPT 分区表 | `gpt.img` |
| `./build.sh <project> system` | 构建系统分区 | `system.emmc` |

### eMMC 分区配置

分区定义位于 `device/<project>/config/partition_emmc.xml`：

```xml
<physical_partition type="emmc">
    <partition label="boot"     size_in_kb="16384"  file="boot.emmc" />
    <partition label="rootfs"   size_in_kb="70656"  file="rootfs.emmc" />
    <partition label="system"   size_in_kb="40960"  file="system.emmc" type="ext4" />
    <partition label="data"     size_in_kb="3145728" mountpoint="/mnt/data" type="ext4"/>
</physical_partition>
```

| 分区 | 大小 | 用途 |
|------|------|------|
| boot | 16MB | 启动镜像（Kernel + DTB） |
| rootfs | 69MB | 根文件系统 |
| system | 40MB | 系统库和 SDK |
| data | 3GB | 用户数据分区 |

---

## 驱动开发

### 驱动列表

| 驱动目录 | 说明 | 关键文件 |
|----------|------|----------|
| `drm_debug/` | DRM 显示框架调试 | modeset、pattern、direct_show、debugfs |
| `eth_phy/` | 以太网 PHY 芯片驱动框架 | PHY 通用接口 |
| `ina_219/` | INA219 电流/功率传感器 | I2C 接口 |
| `lt9211/` | LT9211 Serializer 驱动 | MIPI 到 LVDS 转换 |
| `max96745/` | MAX96745 Deserializer 驱动 | GMSL 协议 |
| `rotary_encoder/` | 旋转编码器设备驱动 | 增量式编码器 |

### 编译驱动

```bash
./build.sh <project_name> kernel_driver
```

### DRM 驱动详解

`drm_debug/` 模块包含多个 DRM 显示调试示例：

- **drm_debug.c** - 基础 DRM 驱动框架
- **drm_debug_modeset.c** - 显示模式（分辨率、刷新率）设置
- **drm_debug_pattern.c** - 测试图案生成（颜色条、灰阶等）
- **drm_debugfs.c** - DebugFS 调试接口
- **drm_debug_direct_show.c** - 直接帧缓冲显示

---

## 镜像输出

构建完成后，产物位于 `build_out/<project_name>/image/` 目录：

```
build_out/rcar_e3_control_box/image/
├── rawimages/
│   ├── u-boot.bin              # U-Boot 镜像
│   ├── Image                   # Linux 内核镜像（ARM64）
│   ├── r8a77990-ebisu.dtb      # 设备树 Blob
│   ├── ramfs.cpio.gz           # RAM Disk
│   ├── boot.emmc               # 启动分区镜像
│   ├── rootfs.emmc             # 根文件系统镜像
│   ├── system.emmc             # 系统镜像
│   └── gpt.img                 # GPT 分区表
│
└── *.img                        # 最终可烧录镜像
```

---

## 配置选项

### 编译模式

项目支持 Debug/Release 两种编译模式：

```bash
# Debug 模式（包含调试符号）
export CONFIG_BUILD_FOR_DEBUG=y
./build.sh <project_name>

# Release 模式（优化编译，默认）
./build.sh <project_name>
```

### 架构选择

在 `device/<project>/config/project_config` 中设置：

```bash
# ARM64（默认）
export SDK_VERSION=64bit

# ARM32
export SDK_VERSION=32bit
```

### 内核配置

预定义的内核配置文件：

| 配置文件 | 用途 |
|----------|------|
| `kernel_debug_defconfig` | Debug 构建配置 |
| `kernel_release_defconfig` | Release 构建配置 |

### U-Boot 配置

| 配置文件 | 用途 |
|----------|------|
| `uboot_debug_defconfig` | Debug 构建配置 |
| `uboot_release_defconfig` | Release 构建配置 |

---

## 工具链

项目自动下载并配置 Linaro GCC 7.4.1 交叉编译工具链：

| 架构 | 工具链名称 |
|------|------------|
| ARM64 | `gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu` |
| ARM32 | `gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf` |

工具链存放路径：`./tool_chain/`

---

## 常见问题

### Q1: 构建失败，提示 toolchain 下载失败？

手动下载工具链：

```bash
wget https://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/aarch64-linux-gnu/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz
tar -xf gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz -C ./tool_chain
```

### Q2: 如何清理构建产物？

```bash
rm -rf build_out/<project_name>
```

### Q3: 如何查看构建详细日志？

```bash
./build.sh <project_name> 2>&1 | tee build.log
```

### Q4: 内存不足构建失败？

```bash
# 减少并行编译线程数
export COREN=2
./build.sh <project_name>
```

---

## 相关资源

| 资源 | 链接 |
|------|------|
| QEMU 模拟器 | https://www.qemu.org/ |
| Buildroot | https://buildroot.org/ |
| U-Boot | https://www.denx.de/wiki/U-Boot/ |
| Linux Kernel | https://www.kernel.org/ |
| Renesas R-Car | https://www.renesas.com/us/en/products/automotive/rcar |
| Linaro Toolchain | https://releases.linaro.org/components/toolchain/binaries/ |

---

