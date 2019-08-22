/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-23     Ernest       first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>

#define LOG_TAG              "main"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

extern void ulog_example_a(void);
extern void ulog_example_b(void);

int main(void)
{
    uint8_t i, buf[128];
    /* 在数组内填充上数字 */
    for (i = 0; i < sizeof(buf); i++)
    {
        buf[i] = i;
    }
    /* 以 hex 格式 dump 数组内的数据，宽度为 16 */
    LOG_HEX("buf_dump", 16, buf, sizeof(buf));
    /* 输出不带任何格式的日志 */		
    LOG_RAW("RAW : This is raw log.\n");

    /* 输出标签为 example_a 的不同级别日志*/
    ulog_example_a();
    /* 输出标签为 example_b 的不同级别日志*/
    ulog_example_b();

    return 0;
}
