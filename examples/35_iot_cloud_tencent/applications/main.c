/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-01     ZeroFree     first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>

#include "drv_wifi.h"
#include "wifi_config.h"
#include <easyflash.h>
#include <fal.h>

#include <stdio.h>
#include <stdlib.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    /* 初始化分区表 */
    fal_init();
    /* 初始化 easyflash */
    easyflash_init();
    /* 配置 wifi 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    /* 初始化自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 查询 wlan 是否处于连接状态 */
    while (rt_wlan_is_ready() != RT_TRUE)
    {
        rt_thread_delay(1000);
    }

    return 0;
}
