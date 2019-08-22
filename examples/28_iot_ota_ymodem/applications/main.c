/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-06-05     armink       first implementation
 */

#include <rtthread.h>
#include <fal.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define APP_VERSION "1.0.0"

int main(void)
{
    fal_init();

    LOG_D("The current version of APP firmware is %s", APP_VERSION);

    return 0;
}
