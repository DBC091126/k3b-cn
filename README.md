# K3b 刻录软件（中文版）

一个界面与 KDE 的 **K3b** 类似的 CD/DVD/BD 刻录工具，**界面完全中文化**，
用 C++ / Qt6 编写，底层调用 Linux 系统已有的刻录工具。

> 该项目为演示与学习用途的中文实现，与 KDE 官方 K3b 项目无关。

## 功能

| 功能 | 说明 |
|------|------|
| 数据光盘项目 | 拖拽添加文件/文件夹、卷标、容量指示条、一键烧录 |
| 音乐 CD 项目 | 添加 WAV/MP3/OGG/FLAC 音轨，自动转码后刻录为 CD |
| 光盘复制 | 数据盘用 `dd` 整盘复制；音频盘用 `cdrdao` 精确复制 |
| ISO 镜像工具 | 用 `genisoimage/mkisofs` 制作镜像；用 `growisofs`/`wodim` 烧录 |
| 介质信息 | 检测介质类型、是否空白/可写/可擦除、容量、厂商型号 |
| 擦除光盘 | 快速/完全擦除 CD-RW、DVD±RW；强制格式化 DVD+RW/BD-RE |
| 刻录进度与日志 | 实时进度条（解析 growisofs/wodim/cdrdao/dd 输出）+ 详细日志 |

## 底层工具依赖

后端自动检测以下工具（用 `PATH` 搜索），缺失时自动进入**演示模式**（模拟进度与日志）：

- `wodim` / `cdrecord` —— CD 写入
- `genisoimage` / `mkisofs` —— ISO 镜像制作
- `growisofs` —— DVD / BD 写入
- `cdrdao` —— 音频盘精确复制
- `dvd+rw-mediainfo`、`dvd+rw-format` —— 介质检测与格式化
- `ffmpeg` —— 非 WAV 音轨转码
- `dd` —— 整盘读取
- `eject` —— 弹出光盘

## 编译

```bash
# 1. 安装依赖（Debian/Ubuntu）
sudo apt-get update
sudo apt-get install -y cmake qt6-base-dev

# 2. 配置并编译
cd k3b-cn
cmake -S . -B build
cmake --build build -j$(nproc)

# 3. 运行
./build/k3b-cn
```

## 使用提示

- 烧录前请确认光盘已放入光驱，且介质可写（`介质信息`页可检测）。
- 当前用户在 `cdrom` 组时可直接写 `/dev/sr*`，否则需要 root 权限
  （或在桌面上用 `sudo ./build/k3b-cn`）。
- 音乐 CD 只接受 WAV/MP3/OGG/FLAC 等格式；非 WAV 格式需要 `ffmpeg` 转码。
- 数据项目默认写入卷标可在"卷标"输入框修改（≤16 字符）。
- 项目可通过 `文件 → 保存项目` 存为 `.k3bcn` 文件，随时打开继续。
- **8cm 迷你盘**：容量自动按 ATIP lead-out 精确识别（如 219 MB / 25:02），
  数据项目容量条与音乐 CD 时长限制都会按真实容量计算。
- **便携 USB 光驱**：多数不支持远程关仓，弹出托盘后请手动推回。

## 已知问题与修复

**wodim 缓冲区 mmap 失败**（`Cannot get mmap for ... on /dev/zero`）：
cdrkit 的 wodim 在默认参数下会用 `mmap` 申请约 12.5MB 缓冲区，
在内存锁（`RLIMIT_MEMLOCK`，普通用户默认 8MB）受限时会直接失败退出。
本程序对所有 wodim 调用显式传入 `fs=6000`（FIFO 6MB），
使 wodim 走普通内存分配路径，规避该问题（已在便携光驱 + CD-R 上实测通过）。
如需恢复大缓冲区，可在 `/etc/security/limits.conf` 为用户添加
`<用户名> - memlock unlimited` 并重新登录。

**老光驱 DAO 模式拒收 CUE sheet**：本程序数据盘与音频盘统一使用
TAO 模式刻录（兼容性最好）；音频轨道间会保留标准 2 秒间隔。

## 目录结构

```
k3b-cn/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp            入口
    ├── Types.h             公共类型（作业类型、介质状态、格式化工具）
    ├── Settings.h/.cpp     刻录设置（QSettings 持久化）
    ├── JobRunner.h/.cpp    作业执行器（QProcess + 进度解析 + 演示模式）
    ├── Backend.h/.cpp      后端（工具/介质检测、刻录操作编排）
    ├── Projects.h/.cpp     数据项目 / 音乐项目模型
    ├── Icons.h             程序内绘制的图标
    ├── MainWindow.h/.cpp   主窗口（菜单/工具栏/导航/状态栏）
    └── pages/              各功能页面
```
