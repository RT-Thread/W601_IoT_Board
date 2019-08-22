# AHT10

## 简介

AHT10 软件包提供了使用温度与湿度传感器 `aht10` 基本功能，并且提供了软件平均数滤波器可选功能。并且本软件包新的版本已经对接到了 Sensor 框架，通过 Sensor 框架，开发者可以快速的将此传感器驱动起来。若想查看**旧版软件包**的 README 请点击[这里](README_OLD.md)。

基本功能主要由传感器 `aht10` 决定：在输入电压为 `1.8v-3.3v` 范围内，测量温度与湿度的量程、精度如下表所示

| 功能 | 量程 | 精度 |
| ---- | ---- | ---- |
| 温度 | `-40℃ - 85℃` |`±0.5℃`|
| 湿度 | `0% - 100%` |`±3%`|

## 支持情况

| 包含设备 | 温度 | 湿度 |
| ---- | ---- | ---- |
| **通信接口** |          |        |
| IIC      | √        | √      |
| **工作模式**     |          |        |
| 轮询             | √        | √      |
| 中断             |          |        |
| FIFO             |          |        |

## 使用说明

### 依赖

- RT-Thread 4.0.0+
- Sensor 组件
- IIC 驱动：aht10 设备使用 IIC 进行数据通讯，需要系统 IIC 驱动支持

### 获取软件包

使用 aht10 软件包需要在 RT-Thread 的包管理中选中它，具体路径如下：

```
RT-Thread online packages  --->
  peripheral libraries and drivers  --->
    sensors drivers  --->
            aht10: digital humidity and temperature sensor aht10 driver library. 
                         [ ]   Enable average filter by software         
                               Version (latest)  --->
```

**Enable average filter by software**：选择后会开启采集温湿度软件平均数滤波器功能。

**Version**：软件包版本选择，默认选择最新版本。

### 使用软件包

aht10 软件包初始化函数如下所示：

```
int rt_hw_aht10_init(const char *name, struct rt_sensor_config *cfg)；
```

该函数需要由用户调用，函数主要完成的功能有，

- 设备配置和初始化（根据传入的配置信息配置接口设备）；
- 注册相应的传感器设备，完成 aht10 设备的注册；

#### 初始化示例

```c
#include "sensor_asair_aht10.h"
#define AHT10_I2C_BUS  "i2c4"

int rt_hw_aht10_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name  = AHT10_I2C_BUS;
    cfg.intf.user_data = (void *)AHT10_I2C_ADDR;
    
    rt_hw_aht10_init("aht10", &cfg);

    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_aht10_port);
```

## 注意事项

测试中发现传感器 `aht10` 为不标准 I2C 设备，总线上出现数据与该器件地址相同，即使没有开始信号，也会响应，导 SDA 死锁。所以，建议用户给 AHT10 独立一个 I2C 总线。

## 联系人信息

维护人:

- 维护：[Ernest](https://github.com/ErnestChen1)
- 主页：https://github.com/RT-Thread-packages/aht10