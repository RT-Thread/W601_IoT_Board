/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2019-05-15     Ernest Chen    first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 配置 LED 灯引脚 */
#define LED_PIN (30)

static char led_status = '0';

int main(void)
{
    /* 设置 LED 引脚为输出模式 */
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);
    /* LED 默认关闭 */
    rt_pin_write(LED_PIN, PIN_HIGH);

    return 0;
}

static at_result_t led_parameter(void)
{
    /* 查询 LED 的可设置状态 */
    at_server_printf("AT+LED= 0, 1");
    return AT_RESULT_OK;
}

static at_result_t led_query(void)
{
    /* 查询 LED 的状态 */
    at_server_printf("AT+LED: %c", led_status);
    return AT_RESULT_OK;
}

static at_result_t led_setup(const char *args)
{
    at_result_t  result;
    /* 解析得到结果存入 led_status */
    if (at_req_parse_args(args, "=%c", &led_status) > 0)
    {
        if ('0' == led_status)
        {
            /* LED 灯灭 */
            rt_pin_write(LED_PIN, PIN_HIGH);
        }
        else
        {
            /* LED 灯亮 */
            rt_pin_write(LED_PIN, PIN_LOW);
        }
        result = AT_RESULT_OK;
    }
    else
    {
        result = AT_RESULT_PARSE_FAILE;
    }
    return result;
}

AT_CMD_EXPORT("AT+LED", "=<value>", led_parameter, led_query, led_setup, RT_NULL);
