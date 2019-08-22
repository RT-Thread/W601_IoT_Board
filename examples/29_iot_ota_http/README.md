# HTTP 协议固件升级例程

## 例程说明

HTTP 是一种超文本传输协议，采用请求应答的通讯模式，可以基于通用 socket 实现极简客户端程序，广泛应用在互联网上。HTTP 请求应答的方式，及其很小的客户端代码量，也使其很方便地应用在物联网设备中，用于与服务器进行数据交互，以及 OTA 固件下载。

本例程基于 HTTP 客户端实现 **HTTP OTA 固件下载器**，通过 HTTP 协议从 HTTP 服务器下载升级固件到设备。HTTP 客户端代码参考 RT-Thread [**WebClient 软件包**](https://github.com/RT-Thread-packages/webclient) 。

## 背景知识

参考 Ymodem 固件升级例程。

## 硬件说明

本例程使用到的硬件资源如下所示：

- UART0(Tx: PA4; Rx: PA5)
- 片内 FLASH (1MBytes)
- 片外 Nor Flash (16MBytes)

## 分区表

| 分区名称   | 存储位置   | 起始地址   | 分区大小 | 结束地址   | 说明                  |
| ---------- | ---------- | ---------- | -------- | ---------- | --------------------- |
| app        | 片内 FLASH | 0x08010100 | 950K     | 0x080fd900 | app 应用程序存储区    |
| easyflash  | Nor FLASH  | 0x08000000 | 1M       | 0x08100000 | easyflash 存储区      |
| download   | Nor FLASH  | 0x08100000 | 1M       | 0x08200000 | download 下载存储区   |
| font       | Nor FLASH  | 0x08200000 | 7M       | 0x08900000 | font 字库分区         |
| filesystem | Nor FLASH  | 0x08900000 | 7M       | 0x09000000 | filesystem 文件系统区 |

分区表定义在 **`bootloader 程序`** 中，如果需要修改分区表，则需要修改 bootloader 程序。目前不支持用户自定义 bootloader，如果有商用需求，请联系 **RT-Thread** 获取支持。

## 软件说明

**http 例程**位于 `/examples/29_iot_ota_http` 目录下，重要文件摘要说明如下所示：

| 文件                    | 说明                                               |
| :---------------------- | :------------------------------------------------- |
| applications/main.c     | app 入口                                           |
| applications/ota_http.c | http ota 应用，基于 HTTP 协议实现 OTA 固件下载业务 |
| ports/fal               | Flash 抽象层软件包（fal）的移植文件                |
| packages/fal            | fal 软件包                                         |
| packages/webclient      | webclient 软件包，实现 HTTP 客户端                 |

HTTP 固件升级流程如下所示：

1. 打开 **tools/MyWebServer** 软件，并配置本机 IP 地址和端口号，选择存放升级固件的目录
2. 在 MSH 中使用 `http_ota` 命令下载固件到 download 分区
3. bootloader 对 OTA 升级固件进行校验、解密和搬运（搬运到 app 分区）
4. 程序从 bootloader 跳转到 app 分区执行新的固件

### 程序说明

HTTP OTA 固件下载器程序代码在 `/examples/29_iot_ota_http/applications/ota_http.c` 文件中，仅有三个 API 接口，介绍如下：

**print_progress 函数**

```c
static void print_progress(size_t cur_size, size_t total_size);
```

该函数用于打印文件的下载进度。

**http_ota_fw_download 函数**

```c
static int http_ota_fw_download(const char* uri);
```

http_ota_fw_download 函数基于 webclient API 实现了从指定的 **uri** 下载文件的功能，并将下载的文件存储到 **download 分区**。

**uri** 格式示例： `http://192.168.1.10:80/rt-thread.rbl`。非 80 端口需要用户指定。如果使用了 TLS 加密连接，请使用 `https://192.168.1.10:80/rt-thread.rbl`。

**http_ota 函数**

```c
void http_ota(uint8_t argc, char **argv);
MSH_CMD_EXPORT(http_ota, OTA by http client: http_ota [url]);
```

HTTP OTA 入口函数，使用 `MSH_CMD_EXPORT` 函数将其导出为 `http_ota` 命令。

`http_ota` 命令需要传入固件下载地址，示例：`http_ota http://192.168.1.10:80/rt-thread.rbl`。

## 运行

本例程演示使用 http OTA 功能烧录 v2.0.0 版本的 app 程序。

### 编译&下载

- **MDK**：双击 `project.uvprojx` 打开 MDK5 工程，执行编译。

编译完成后，将固件下载至开发板。

### 运行效果

按下复位按键重启开发板，正常运行后，可以看到当前版本为 v1.0.0。

```shell
 \ | /
- RT -     Thread Operating System
 / | \     4.0.1 build Jun  6 2019
 2006 - 2019 Copyright by rt-thread team
lwIP-2.0.2 initialized!
[SFUD] Find a Winbond flash chip. Size is 16777216 bytes.
[SFUD] w25q128 flash device is initialize success.
[I/sal.skt] Socket Abstraction Layer initialize success.
[I/WLAN.dev] wlan init success
[I/WLAN.lwip] eth device init ok name:w0
[D/FAL] (fal_flash_init:61) Flash device |              w60x_onchip | addr: 0x08000000 | len: 0x00100000 | blk_size: 0x00001000 |initialized finish.
[D/FAL] (fal_flash_init:61) Flash device |                 norflash | addr: 0x00000000 | len: 0x01000000 | blk_size: 0x00001000 |initialized finish.
[D/FAL] (fal_partition_init:176) Find the partition table on 'w60x_onchip' offset @0x0000f0c8.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name       | flash_dev   |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | easyflash  | norflash    | 0x00000000 | 0x00100000 |
[I/FAL] | app        | w60x_onchip | 0x00010100 | 0x000ed800 |
[I/FAL] | download   | norflash    | 0x00100000 | 0x00100000 |
[I/FAL] | font       | norflash    | 0x00200000 | 0x00700000 |
[I/FAL] | filesystem | norflash    | 0x00900000 | 0x00700000 |
[I/FAL] =============================================================
[I/FAL] RT-Thread Flash Abstraction Layer (V0.3.0) initialize success.
[Flash] (packages\EasyFlash-v3.3.0\src\ef_env.c:152) ENV start address is 0x00000000, size is 4096 bytes.
[Flash] (packages\EasyFlash-v3.3.0\src\ef_env.c:821) Calculate ENV CRC32 number is 0xD6363A94.
[Flash] (packages\EasyFlash-v3.3.0\src\ef_env.c:833) Verify ENV CRC32 result is OK.
[Flash] EasyFlash V3.3.0 is initialize success.
[Flash] You can get the latest version on https://github.com/armink/EasyFlash .
[D/main] The current version of APP firmware is 1.0.0
msh />[I/WLAN.mgnt] wifi connect success ssid:realthread
[I/WLAN.lwip] Got IP address : 192.168.12.92
```

### 启动 HTTP OTA 升级

1. 解压 `/tools/MyWebServer.zip` 到当前目录（解压后有 **/tools/MyWebServer** 目录）

2. 双击 `project.uvprojx` 打开 MDK5 工程，修改本例程 mian.c 中的 `APP_VERSION` 为 2.0.0 ，执行编译。编译完成后，在本示例的目录下会生成 Bin 文件夹，其中含有 OTA 升级固件 `rtthread.rbl` 。

3. 打开 **/tools/MyWebServer** 目录下的 **MyWebServer.exe** 软件

   配置 MyWebServer 软件，选择 OTA 固件（rbl 文件）的路径，设置本机 IP 和 端口号，并启动服务器，如下图所示：

   ![启动 MyWebServer 软件](../../docs/figures/29_iot_ota_http/mywebserver.png)

4. 连接开发板串口，复位开发板，进入 MSH 命令行

5. 在设备的命令行里输入 **`http_ota http://192.168.1.10:80/rt-thread.rbl`** 命令启动 HTTP OTA 升级

   根据您的 MyWebServer 软件的 IP 和端口号配置修改 `http_ota` 命令。

6. 设备升级过程

   输入命令后，会擦除 download 分区，下载升级固件。下载过程中会打印下载进度条。

```
msh />http_ota http://192.168.1.10:80/rt-thread.rbl
[I/http_ota] Start erase flash (download) partition!
[I/http_ota] Erase flash (download) partition success!
[I/http_ota] Download: [====================================================================================================] 100%
[I/http_ota] Download firmware to flash success.
[I/http_ota] System now will restart...
PPPPPPPPPPPPPPPCCCCCCCCCCCCCCCCCCCCCCCCCC
```

**HTTP OTA** 下载固件完成后，手动复位硬件。

设备重启后，**bootloader** 会对升级固件进行合法性和完整性校验，验证成功后将升级固件从**download** 分区搬运到目标分区（这里是 **app** 分区）。

升级成功后设备状态如下所示：

```shell
[SFUD]Find a Winbond flash chip. Size is 16777216 bytes.
[SFUD]norflash flash device is initialize success.
[I/FAL] RT-Thread Flash Abstraction Layer (V0.4.0) initialize success.
[I/OTA] RT-Thread OTA package(V0.2.3) initialize success.
[D/OTA] (ota_main:62) check upgrade...
[I/OTA] Verify 'download' partition(fw ver: 1.1.0, timestamp: 1559803627) success.
[I/OTA] OTA firmware(app) upgrade(1.1.0->1.1.0) startup.
[I/OTA] The partition 'app' is erasing.
[I/OTA] The partition 'app' erase success.
[I/OTA] OTA Write: [==========================================================] 100%
[D/OTA] (ota_main:105) jump to APP!
redirect_addr:8010100, stk_addr:2000ECD8, len:972800

 \ | /
- RT -     Thread Operating System
 / | \     4.0.1 build Jun  6 2019
 2006 - 2019 Copyright by rt-thread team
lwIP-2.0.2 initialized!
[SFUD] Find a Winbond flash chip. Size is 16777216 bytes.
[SFUD] w25q128 flash device is initialize success.
[I/sal.skt] Socket Abstraction Layer initialize success.
[I/WLAN.dev] wlan init success
[I/WLAN.lwip] eth device init ok name:w0
[D/FAL] (fal_flash_init:61) Flash device |              w60x_onchip | addr: 0x08000000 | len: 0x00100000 | blk_size: 0x00001000 |initialized finish.
[D/FAL] (fal_flash_init:61) Flash device |                 norflash | addr: 0x00000000 | len: 0x01000000 | blk_size: 0x00001000 |initialized finish.
[D/FAL] (fal_partition_init:176) Find the partition table on 'w60x_onchip' offset @0x0000f0c8.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name       | flash_dev   |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | easyflash  | norflash    | 0x00000000 | 0x00100000 |
[I/FAL] | app        | w60x_onchip | 0x00010100 | 0x000ed800 |
[I/FAL] | download   | norflash    | 0x00100000 | 0x00100000 |
[I/FAL] | font       | norflash    | 0x00200000 | 0x00700000 |
[I/FAL] | filesystem | norflash    | 0x00900000 | 0x00700000 |
[I/FAL] =============================================================
[I/FAL] RT-Thread Flash Abstraction Layer (V0.3.0) initialize success.
[Flash] (packages\EasyFlash-v3.3.0\src\ef_env.c:152) ENV start address is 0x00000000, size is 4096 bytes.
[Flash] (packages\EasyFlash-v3.3.0\src\ef_env.c:821) Calculate ENV CRC32 number is 0xD6363A94.
[Flash] (packages\EasyFlash-v3.3.0\src\ef_env.c:833) Verify ENV CRC32 result is OK.
[Flash] EasyFlash V3.3.0 is initialize success.
[Flash] You can get the latest version on https://github.com/armink/EasyFlash .
[D/main] The current version of APP firmware is 2.0.0
msh />[I/WLAN.mgnt] wifi connect success ssid:realthread
[I/WLAN.lwip] Got IP address : 192.168.12.92

```

设备升级完成后会自动运行新的固件，从上图中的日志上可以看到，app 固件已经从 **1.0.0 版本**升级到了 **2.0.0 版本**。

**2.0.0 版本**的固件同样是支持 HTTP OTA 下载功能的，因此可以一直使用 HTTP 进行 OTA 升级。用户如何需要增加自己的业务代码，可以基于该例程进行修改。

## 注意事项

- MyWebServer 软件可能会被您的防火墙限制功能，使用前请检查 Windows 防火墙配置

- 串口波特率 115200，无奇偶校验，无流控

- 如果要升级其他 APP 例程，请先将原 APP 例程移植到该例程工程，然后编译除 APP 固件，再进行升级操作

## 引用参考

- 《RT-Thread 编程指南 》: docs/RT-Thread 编程指南.pdf
- 《RT-Thread OTA 用户手册》: docs/UM1004-RT-Thread-OTA 用户手册.pdf
- OTA 说明请参考 **Ymodem 固件升级**章节
- WiFi使用说明请参考 **使用 WiFi Manager 管理、操作 WiFi 网络**章节

