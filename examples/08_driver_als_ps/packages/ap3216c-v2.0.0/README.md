# AP3216C 软件包

## 简介

AP3216C 软件包提供了使用接近感应（ps）与光照强度（als）传感器 `ap3216c` 基本功能，并且本软件包新的版本已经对接到了 Sensor 框架，通过 Sensor 框架，开发者可以快速的将此传感器驱动起来。若想查看**旧版软件包**的 README 请点击[这里](README_OLD.md)。

- **光照强度** ：支持 4 个量程
- **接近感应** ：支持 4 种增益
- **中断触发** ：光照强度及接近感应同时支持 `高于阈值` 或 `低于阈值` 的两种硬件中断触发方式

## 支持情况

| 包含设备     | 光照强度 | 接近感应 |
| ------------ | -------- | -------- |
| **通信接口** |          |          |
| IIC          | √        | √        |
| **工作模式** |          |          |
| 轮询         | √        | √        |
| 中断         |          |          |
| FIFO         |          |          |

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
          ap3216c: a digital ambient light and a proximity sensor ap3216c driver library. 
               [ ]   Enable hardware interrupt.   
               (59)    The number of the sensor hardware interrupt pin. 
               Version (latest)  --->
```

**Enable hardware interrupt**：选择后会开启中断功能。

**The number of the sensor hardware interrupt pin**：中断引脚配置。

**Version**：软件包版本选择，默认选择最新版本。

### 使用软件包

ap3216c 软件包初始化函数如下所示：

```c
int rt_hw_ap3216c_init(const char *name, struct rt_sensor_config *cfg)；
```

该函数需要由用户调用，函数主要完成的功能有，

- 设备配置和初始化（根据传入的配置信息配置接口设备）；
- 注册相应的传感器设备，完成 ap3216c 设备的注册；

#### 初始化示例

```c
#include "sensor_lsc_ap3216c.h"

#define AP3216C_I2C_BUS  "i2c3"

int rt_hw_ap3216c_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name  = AP3216C_I2C_BUS;
    cfg.intf.user_data = (void *)AP3216C_I2C_ADDR;

    rt_hw_ap3216c_init("ap3216c", &cfg);

    return RT_EOK;
}
INIT_ENV_EXPORT(rt_hw_ap3216c_port);
```

## 注意事项

暂无。

## 联系人信息

维护人:

- 维护：[Ernest](https://github.com/ErnestChen1)
- 主页：https://github.com/RT-Thread-packages/ap3216c