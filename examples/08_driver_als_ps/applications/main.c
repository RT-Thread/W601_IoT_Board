/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-31     yangjie      first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "sensor_lsc_ap3216c.h"
#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define ALS_DEV       "li_ap321"
#define PS_DEV        "pr_ap321"

int main(void)
{
    int count = 0;
    rt_device_t als_dev, ps_dev;
    struct rt_sensor_data als_dev_data, ps_dev_data;

    LOG_D("Als Ps Sensor Testing Start...");
    
    /* 查找并打开光强传感器 */
    als_dev = rt_device_find(ALS_DEV);
    if (als_dev == RT_NULL)
    {
        LOG_E("can not find ALS device: %s", ALS_DEV);
        return -RT_ERROR;
    }
    else
    {
        if (rt_device_open(als_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open ALS device failed!");
            return -RT_ERROR;
        }
    }

    /* 查找并打开接近传感器 */
    ps_dev = rt_device_find(PS_DEV);
    if (ps_dev == RT_NULL)
    {
        LOG_E("can not find PS device: %s", PS_DEV);
        return -RT_ERROR;
    }
    else
    {
        if (rt_device_open(ps_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
        {
            LOG_E("open PS device failed!");
            return -RT_ERROR;
        }
    }

    /* 开始读取传感器数据 */
    while (count++ < 20)
    {
        rt_device_read(als_dev, 0, &als_dev_data, 1);
        LOG_D("current brightness: %d.%d(lux).", (int)(als_dev_data.data.light / 10), (int)(als_dev_data.data.light % 10));

        rt_device_read(ps_dev, 0, &ps_dev_data, 1);
        if (ps_dev_data.data.proximity == 0)
        {
            LOG_D("no object approaching.");
        }
        else
        {
            LOG_D("current ps data   : %d.", ps_dev_data.data.proximity);
        }

        rt_thread_mdelay(1000);
    }
    
    rt_device_close(als_dev);
    rt_device_close(ps_dev);
    LOG_D("Als Ps Sensor Testing Ended.");

    return RT_EOK;
}
