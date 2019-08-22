/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-16     yangjie      first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "sensor_asair_aht10.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define TEMP_DEV       "temp_aht"
#define HUMI_DEV       "humi_aht"

int main(void)
{
    int count = 0;
    rt_device_t temp_dev, humi_dev;
    struct rt_sensor_data temp_dev_data, humi_dev_data;

    LOG_D("Temperature and Humidity Sensor Testing Start...");
    rt_thread_mdelay(2000);

    /* 寻找并打开温度传感器 */
    temp_dev = rt_device_find(TEMP_DEV);
    if (temp_dev == RT_NULL)
    {
        LOG_E("can not find TEMP device: %s", TEMP_DEV);
        return RT_ERROR;
    }
    else
    {
        if (rt_device_open(temp_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open TEMP device failed!");
            return RT_ERROR;
        }
    }

    /* 寻找并打开湿度传感器 */
    humi_dev = rt_device_find(HUMI_DEV);
    if (humi_dev == RT_NULL)
    {
        LOG_E("can not find HUMI device: %s", HUMI_DEV);
        return RT_ERROR;
    }
    else
    {
        if (rt_device_open(humi_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open HUMI device failed!");
            return RT_ERROR;
        }
    }

    while (count++ < 20)
    {
        rt_device_read(temp_dev, 0, &temp_dev_data, 1);
        LOG_D("temperature: %d.%d C", (int)(temp_dev_data.data.temp / 10), (int)(temp_dev_data.data.temp % 10));

        rt_device_read(humi_dev, 0, &humi_dev_data, 1);
        LOG_D("humidity   : %d.%d %%", (int)(humi_dev_data.data.humi / 10), (int)(humi_dev_data.data.humi % 10));

        rt_thread_mdelay(1000);
    }
    
    rt_device_close(temp_dev);
    rt_device_close(humi_dev);
    LOG_D("Temperature and Humidity Sensor Testing Ended.");

    return RT_EOK;
}
