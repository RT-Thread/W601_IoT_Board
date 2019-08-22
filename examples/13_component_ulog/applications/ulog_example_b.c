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

#define LOG_TAG              "example.b"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

void ulog_example_b(void)
{
    /* 输出不同级别的日志 */
    LOG_D("Debug       : This is debug log.");
    LOG_I("Information : This is information log.");
    LOG_W("Warning     : This is warning log.");
    LOG_E("Error       : This is error log.");
}
MSH_CMD_EXPORT(ulog_example_b, run ulog example b)
