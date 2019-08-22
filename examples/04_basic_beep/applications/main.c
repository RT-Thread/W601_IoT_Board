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
#include "board.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

void beep_ctrl(void *args)
{
    if(rt_pin_read(PIN_KEY0) == PIN_LOW)
    {
        rt_pin_write(PIN_BEEP, PIN_HIGH);
        LOG_D("KEY0 pressed. beep on");
    }
    else
    {
        rt_pin_write(PIN_BEEP, PIN_LOW);  
        LOG_D("beep off");        
    }
}

int main(void)
{
    /* 设置 KEY0 引脚为输入模式 */
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT);
    /* KEY0 引脚绑定中断回调函数 */
    rt_pin_attach_irq(PIN_KEY0, PIN_IRQ_MODE_RISING_FALLING, beep_ctrl, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_ENABLE);
    
    /* 设置 BEEP 引脚为输出模式 */
    rt_pin_mode(PIN_BEEP, PIN_MODE_OUTPUT);
    /* 默认蜂鸣器不鸣叫 */
    rt_pin_write(PIN_BEEP, PIN_LOW);

    return 0;
}
