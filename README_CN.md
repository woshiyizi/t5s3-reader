# T5S3 Reader

[![PlatformIO Build](https://github.com/ShallowGreen123/t5s3-reader/actions/workflows/platformio-build.yml/badge.svg)](https://github.com/ShallowGreen123/t5s3-reader/actions/workflows/platformio-build.yml)

[English](README.md) | 中文

适用于 **LilyGo T5 ePaper S3 / T5S3 4.7 寸墨水屏设备** 的电子书阅读器固件。

本项目基于 CrossPoint Reader 的代码和设计继续改造，重点适配 LilyGo T5S3 硬件，并针对 EPUB 插图、TXT 打开速度、低功耗关机、开机刷新等问题做了优化。

## 致谢

感谢 [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) 项目。这个固件继承了 CrossPoint 的活动页面架构、阅读器逻辑、设置系统、SD 卡缓存、Web 文件传输等大量基础工作。

本仓库不是 CrossPoint 官方项目，也不隶属于 LilyGo。它是面向 T5S3 设备的适配和实验版本。

## 使用的设备

当前目标设备：

- **开发板/设备**：[LilyGo T5 ePaper S3](https://github.com/Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO)
- **主控**：ESP32-S3
- **屏幕**：4.7 寸 E-Paper，物理分辨率 960 x 540，默认竖屏逻辑分辨率 540 x 960
- **存储**：microSD 卡，用于放置电子书、缓存、设置和截图
- **输入**：前排按键、侧边按键、电源键、复位键、触摸屏

PlatformIO 环境名为 `default`，板卡定义为 `T5-ePaper-S3`。

## 功能

- 支持 EPUB 阅读，包括章节解析、排版、阅读进度和插图显示。
- 支持 TXT / Markdown 文本阅读。
- 支持 XTC 文件阅读。
- 支持 BMP 图片查看。
- 支持最近阅读、文件浏览、阅读缓存和封面/睡眠图。
- 支持 Wi-Fi 文件上传和 Web 文件管理。
- 支持字体、字号、行距、边距、屏幕方向、刷新模式等设置。
- 支持长时间无操作自动关机；插入 USB 时不会自动关机。
- 支持阅读页截图，截图保存到 SD 卡 `screenshots/` 目录。

## 准备工作

你需要：

- LilyGo T5 ePaper S3 设备
- microSD 卡
- USB-C 数据线
- Python 3
- PlatformIO Core，或 VS Code + PlatformIO 插件

安装 PlatformIO Core：

```bash
python -m pip install platformio==6.1.19
```

获取源码后进入仓库根目录：

```bash
git clone <仓库地址>
cd z-T5S3-Reader
```

## 如何下载程序到设备

### 方式一：使用 LILYGO Spark，推荐

1. 下载并打开 [LILYGO Spark](https://lilygo.cc/en-us/pages/lilygo-spark?srsltid=AfmBOoorTB7ptFu2LQNLRnoI2SA0zBGJTN6JpI9J3hmHEkKhBQSmeu0Y)。
2. 搜索你的设备，并下载 `corsspoint_lilygo_t5s3_e_paper` 程序。

![LILYGO Spark 固件下载](./docs/README_img/lilygo_spark.png)

### 方式二：使用 PlatformIO 下载

1. 将设备通过 USB-C 连接电脑。
2. 在仓库根目录编译固件：

```bash
pio run -e default
```

3. 下载到设备：

```bash
pio run -e default -t upload
```

4. 如果无法进入下载模式，可以按住设备的 BOOT 键，再按一下 RESET，或按住 BOOT 后重新插入 USB，然后重新执行上传命令。

5. 如需查看串口日志：

```bash
pio device monitor -b 115200
```

### 方式三：使用 esptool 手动刷入

安装 esptool：

```bash
python -m pip install esptool
```

编译后固件位于：

```text
.pio/build/default/firmware.bin
```

只刷应用固件：

```bash
esptool.py --chip esp32s3 --port COMx --baud 921600 write_flash 0x10000 .pio/build/default/firmware.bin
```

如果是空白设备或需要完整恢复，可以同时刷入 bootloader、分区表和固件：

```bash
esptool.py --chip esp32s3 --port COMx --baud 921600 write_flash \
  0x0000 .pio/build/default/bootloader.bin \
  0x8000 .pio/build/default/partitions.bin \
  0x10000 .pio/build/default/firmware.bin
```

将 `COMx` 替换成你的串口，例如 Windows 下的 `COM5`，Linux 下的 `/dev/ttyACM0`，macOS 下的 `/dev/cu.usbmodem*`。

## SD 卡和电子书

将电子书直接放到 SD 卡根目录，或按自己的习惯创建文件夹分类。

推荐结构：

```text
/
  Books/
    book.epub
    novel.txt
  .sleep/
    sleep.bmp
```

固件会在 SD 卡上创建 `.crosspoint/` 目录，用于保存设置、阅读进度、缓存和封面缩略图。若遇到异常缓存或反复崩溃，可以备份后删除 `.crosspoint/` 让系统重新生成。

## 设备如何操作

### 基础按键

| 按键 | 功能 |
| --- | --- |
| BOOT | 短按：上一项 / 阅读时上一页 |
| IO48 | 短按：下一项 / 阅读时下一页 |
| BOOT | 长按：确认 / 打开 |
| IO48 | 长按：关机 |
| PWR | 打开设备电源 |
| RTS | 复位 |
| HOME | 回到主页面 |

### 开机和关机

- 长按 `PWR` 键开机。
- 长按 `IO48` 键关机。
- 长时间无操作且未插 USB 时，设备会自动进入关机/低功耗状态。
- 如果设备无响应，可以按 RESET 后重新长按电源键启动。

### 主页面

主页面可以进入：

- Continue Reading：继续阅读最近一本书。
- Browse Files：浏览 SD 卡文件。
- Recent Books：最近阅读列表。
- File Transfer：通过 Wi-Fi 上传书籍。
- Settings：设置。

使用 Left/Right 或 Up/Down 移动选择，Confirm 打开，Back 返回。

### 文件浏览

- Left / Up：向上移动。
- Right / Down：向下移动。
- Confirm：打开文件或文件夹。
- Back：返回上一级或回到主页。
- 长按 Confirm：删除选中的文件，系统会再次确认。

### 阅读页面

- Right 或 Down：下一页。
- Left 或 Up：上一页。
- Confirm：打开阅读菜单。
- Back：退出阅读并回到主页。
- 长按 Back：退出阅读并回到文件浏览。
- 长按翻页键：按设置执行章节跳转或其他长按行为。
- Power + Down：截图，保存到 SD 卡 `screenshots/` 目录。

### Wi-Fi 上传书籍

1. 在主页面进入 `File Transfer`。
2. 选择并连接 Wi-Fi。
3. 屏幕会显示一个访问地址。
4. 在电脑或手机浏览器打开该地址。
5. 上传 EPUB、TXT 等文件到 SD 卡。
6. 上传完成后，按 Back 退出文件传输模式。

## 常用设置

在 `Settings` 中可以调整：

- 字体、字号、行距、页边距。
- 阅读方向：竖屏、横屏、倒置等。
- 刷新模式：质量优先、平衡、快速。
- EPUB 插图显示方式：显示插图、占位、隐藏。
- 睡眠/关机时间。
- 睡眠屏幕：默认图、空白、自定义 BMP、书籍封面。
- 按键映射。
- Wi-Fi 网络。

## 说明

这个固件仍在持续调整中。墨水屏刷新、图片解码、TXT 大文件加载、功耗和电量计都和具体硬件状态有关，如果遇到异常，请尽量提供串口日志、复现文件和操作步骤。

再次感谢 CrossPoint Reader 项目和相关开源库作者。
