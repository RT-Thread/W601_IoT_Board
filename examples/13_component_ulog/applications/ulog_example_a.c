/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-02     armink       the first version
 */

#include <rtthread.h>

#define LOG_TAG              "example.a"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

void ulog_example_a(void)
{
    char *RTOS = "RT-Thread is an open source IoT operating system from China.";

    /* 输出不同级别的日志 */
    LOG_D("Debug       : %s", RTOS);  /* 调试日志 */
    LOG_I("Information : %s", RTOS);  /* 信息日志 */
    LOG_W("Warning     : %s", RTOS);  /* 警告日志 */
    LOG_E("Error       : %s", RTOS);  /* 错误日志 */

}
MSH_CMD_EXPORT(ulog_example_a, run ulog example a)
