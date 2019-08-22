/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-16     armink       first implementation
 * 2019-06-06     yangjie      update to http ota example
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "wifi_config.h"
#include "board.h"
#include "fal.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define APP_VERSION  "1.0.0"

int main(void)
{    
    /* 配置 WIFI 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);

    /* 配置 WIFI 自动连接 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    LOG_D("The current version of APP firmware is %s", APP_VERSION);

    return 0;
}
