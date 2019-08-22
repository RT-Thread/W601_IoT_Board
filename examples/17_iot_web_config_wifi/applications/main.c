/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-22     Ernest       first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "oneshot.h"
#include "drv_wifi.h"
#include "string.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define NET_READY_TIME_OUT       (rt_tick_from_millisecond(100 * 1000))
struct rt_semaphore  web_sem;
static char ssid[64];
static char passwd[64];

static void oneshot_result_cb(int state, unsigned char *ssid_i, unsigned char *passwd_i)
{
    char *ssid_temp = (char *)ssid_i;
    char *passwd_temp = (char *)passwd_i;

    if (RT_TRUE == rt_wlan_ap_is_active())
        rt_wlan_ap_stop();

    /* 配网回调超时返回 */
    if (state != 0)
    {
        LOG_E("Receive wifi info timeout(%d). exit!", state);
        return;
    }
    if (ssid_temp == RT_NULL)
    {
        LOG_E("SSID is NULL. exit!");
        return;
    }
    LOG_D("Receive ssid:%s passwd:%s", ssid_temp == RT_NULL ? "" : ssid_temp, passwd_temp == RT_NULL ? "" : passwd_temp);

    strncpy(ssid, ssid_temp, strlen(ssid_temp));
    if (passwd_temp)
    {
        strncpy(passwd, passwd_temp, strlen(passwd_temp));
    }
    /* 通知 ssid 与 key 接收完成 */
    rt_sem_release(&web_sem);
}

int main(void)
{
    rt_err_t result = RT_EOK;
    /* 配置 wifi 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    rt_wlan_set_mode(RT_WLAN_DEVICE_AP_NAME, RT_WLAN_AP);
    rt_sem_init(&web_sem, "web_sem", 0, RT_IPC_FLAG_FIFO);

    /* 启动 AP 热点 */
    rt_wlan_start_ap("softap_73b2", NULL);

    rt_thread_mdelay(2000);
    /* 一键配网：APWEB 模式 */
    result = wm_oneshot_start(WM_APWEB, oneshot_result_cb);
    if (result != 0)
    {
        LOG_E("web config wifi start failed");
        return result;
    }

    LOG_D("web config wifi start...");
    result = rt_sem_take(&web_sem, NET_READY_TIME_OUT);
    if (result != RT_EOK)
    {
        LOG_E("connect error or timeout");
        return result;
    };

    /* 配网结束，关闭 AP 热点 */
    if (RT_TRUE == rt_wlan_ap_is_active())
        rt_wlan_ap_stop();

    /* 连接 WiFi */
    result = rt_wlan_connect(ssid, passwd);
    if (result != RT_EOK)
    {
        LOG_E("\nconnect ssid %s key %s error:%d!", ssid, passwd, result);
    };
    return result;
}
