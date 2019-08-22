/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-07     yangjie      first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <drv_lcd.h>
#include <smartconfig.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Airkiss scan http://story.rt-thread.com/api/getAirkiss */
#define IOTB_SMARTCONFIG_API  "http://story.rt-thread.com/api/getAirkiss"

/* Airkiss scan http://story.rt-thread.com/api/getAirkiss */
static void iotb_lcd_show_wechatscan(void)
{
    LOG_D("show [wechat scan page] scan&config wifi");
    lcd_clear(WHITE);
    lcd_set_color(WHITE, BLACK);
    lcd_show_string(32,  2, 32, "WeChat Scan");

    /* QR SIZE = (4 * version + 17)*enlargement = 165 */
    lcd_show_qrcode(37, 55, 4, ECC_LOW, IOTB_SMARTCONFIG_API, 5);
}
extern void smartconfig_demo(void);

int main(void)
{
    rt_err_t result;
    /* 清屏 */
    lcd_clear(WHITE);
    /* 配置 wifi 工作模式 */
    result = rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    if (result == RT_EOK)
    {
        LOG_D("Start airkiss...");
        /* 一键配网 demo */
        smartconfig_demo();
    }
    /* 显示二维码 */
    iotb_lcd_show_wechatscan();

    return 0;
}
