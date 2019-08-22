/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-03-25     balanceTWK   the first version
 */

#include <rthw.h>
#include <rtdevice.h>
#include <board.h>
#include "infrared.h"
#include "drv_infrared.h"

#define DBG_SECTION_NAME     "drv.infrared"
#define DBG_LEVEL     DBG_INFO
#include <rtdbg.h>

#ifdef BSP_USING_INFRARED

static struct infrared_class* infrared;

/* Infrared transmission configuration parameters */
#define PWM_DEV_NAME              "pwm"         /* PWM name */
#define PWM_DEV_CHANNEL           4
#define SEND_HWTIMER              "timer1"     /* Timer name */
#define MAX_SEND_SIZE             1000
#define INFRARED_RECEIVE_PIN      38

struct rt_device_pwm         *pwm_dev;
static rt_uint32_t  infrared_send_buf[MAX_SEND_SIZE];
static rt_device_t           send_time_dev ;
static rt_hwtimerval_t       timeout_s;

static rt_err_t send_timeout_callback(rt_device_t dev, rt_size_t size)
{
    static rt_size_t i = 0;
    if ((infrared_send_buf[i] != 0x5A5A5A5A))/* Determine if it is a stop bit */
    {
        if ((infrared_send_buf[i] & 0xF0000000) == 0xA0000000) /* Determine if it is a carrier signal */
        {
            rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
            timeout_s.sec = 0;
            timeout_s.usec = (infrared_send_buf[i] & 0x0FFFFFFF); /* Get the delay time */
            rt_device_write(send_time_dev, 0, &timeout_s, sizeof(timeout_s));
        }
        else
        {
            rt_pwm_disable(pwm_dev, PWM_DEV_CHANNEL);
            timeout_s.sec = 0;
            timeout_s.usec = (infrared_send_buf[i] & 0x0FFFFFFF) - 70; /* Get the delay time */
            rt_device_write(send_time_dev, 0, &timeout_s, sizeof(timeout_s));
        }

        i++;
    }
    else
    {
        rt_pwm_disable(pwm_dev, PWM_DEV_CHANNEL);
        i = 0;
    }
    return 0;
}

rt_err_t infrared_send_init(void)
{
    rt_err_t ret = RT_EOK;
    rt_hwtimer_mode_t mode;
    rt_uint32_t freq = 1000000;

    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
    if (pwm_dev == RT_NULL)
    {
        LOG_E("pwm sample run failed! can't find %s device!", PWM_DEV_NAME);
        return RT_ERROR;
    }

    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, 26316, 8770);
    rt_pwm_disable(pwm_dev, PWM_DEV_CHANNEL);

    send_time_dev = rt_device_find(SEND_HWTIMER);
    if (send_time_dev == RT_NULL)
    {
        LOG_E("hwtimer sample run failed! can't find %s device!", SEND_HWTIMER);
        return RT_ERROR;
    }
    ret = rt_device_open(send_time_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        LOG_E("open %s device failed!\n", SEND_HWTIMER);
        return ret;
    }
    rt_device_set_rx_indicate(send_time_dev, send_timeout_callback);
    ret = rt_device_control(send_time_dev, HWTIMER_CTRL_FREQ_SET, &freq);
    if (ret != RT_EOK)
    {
        LOG_E("set frequency failed! ret is :%d", ret);
        return ret;
    }
    mode = HWTIMER_MODE_ONESHOT;
    ret = rt_device_control(send_time_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        LOG_E("set mode failed! ret is :%d", ret);
        return ret;
    }
    return ret;
}

static rt_size_t infrared_send(struct ir_raw_data* data, rt_size_t size)
{
    rt_size_t send_size;
    if(size >= MAX_SEND_SIZE)
    {
        LOG_E("The length of the sent data exceeds the MAX_SEND_SIZE.");
        return 0;
    }
    for (send_size = 0; send_size < size; send_size++)
    {
        infrared_send_buf[send_size] = (data[send_size].level<<28) + (data[send_size].us);
    }
    infrared_send_buf[size] = 0x5A5A5A5A;

    timeout_s.sec = 0;
    timeout_s.usec = 500;
    rt_device_write(send_time_dev, 0, &timeout_s, sizeof(timeout_s));
    rt_thread_mdelay(100);
    return send_size;
}

#include "wm_pwm.h"
#include "wm_cpu.h"
#include "wm_io.h"
#include "wm_regs.h"
#include "wm_dma.h"
#include "wm_gpio_afsel.h"
#include "pin_map.h"

static void pwm_isr_callback4(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    int fcount = 0, rcount = 0;
    unsigned int status = tls_reg_read32(HR_PWM_CAP2CTL);

    if (status & 0x80)
    {
        tls_reg_write32(HR_PWM_CAP2CTL, status | 0x18);
    }
    else
    {
        if (status & 0x10)
        {
            if ((status & 0xC0) == 0)
            {
                fcount = ((tls_reg_read32(HR_PWM_CAP2DAT) & 0xFFFF0000) >> 16);
                fcount = fcount / (2.6);
                driver_report_raw_data(IDLE_SIGNAL,fcount);
            }
            tls_reg_write32(HR_PWM_CAP2CTL, status | 0x10);
            LOG_D("fcount = %d", fcount);
        }
        if (status & 0x08)
        {
            if ((status & 0xA0) == 0)
            {
                rcount = (tls_reg_read32(HR_PWM_CAP2DAT) & 0x0000FFFF);
                rcount = rcount / (2.6);
                driver_report_raw_data(CARRIER_WAVE,rcount);
            }
            tls_reg_write32(HR_PWM_CAP2CTL, status | 0x08);
            LOG_D("rcount = %d", rcount);
        }
    }
    /* leave interrupt */
    rt_interrupt_leave();
}

static void pwm_isr_callback0(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    int fcount = 0, rcount = 0;
    unsigned int status = tls_reg_read32(HR_PWM_INTSTS);

    if (status & 0x200)
    {
        tls_reg_write32(HR_PWM_INTSTS, status | 0x60);
    }
    else
    {
        if (status & 0x40)
        {
            if ((status & 0x300) == 0)
            {
                fcount = ((tls_reg_read32(HR_PWM_CAPDAT) & 0xFFFF0000) >> 16);
                fcount = fcount / (2.6);
                driver_report_raw_data(IDLE_SIGNAL,fcount);
            }
            tls_reg_write32(HR_PWM_INTSTS, status | 0x40);
            LOG_D("fcount = %d", fcount);
        }
        else if (status & 0x00000020)
        {
            if ((status & 0x00000280) == 0)
            {
                rcount = (tls_reg_read32(HR_PWM_CAPDAT) & 0x0000FFFF);
                rcount = rcount / (2.6);
                driver_report_raw_data(CARRIER_WAVE,rcount);
            }
            tls_reg_write32(HR_PWM_INTSTS, status | 0x20);
            LOG_D("rcount = %d", rcount);
        }
    }
    /* leave interrupt */
    rt_interrupt_leave();
}

static int pwm_pin_config(u8 channel,rt_base_t pin_number)
{
    enum tls_io_name gpio_pin;
    gpio_pin = wm_get_pin(pin_number);
    switch (channel)
    {
        case 0:
            wm_pwm1_config(gpio_pin);
        break;
        case 1:
            wm_pwm2_config(gpio_pin);
        break;
        case 2:
            wm_pwm3_config(gpio_pin);
        break;
        case 3:
            wm_pwm4_config(gpio_pin);
        break;
        case 4:
            wm_pwm5_config(gpio_pin);
        break;
        default:
            return -1;
    }
    return 0;
}
void pwm_capture_mode_int(u8 channel, rt_base_t pin_number, u32 freq)
{
    tls_sys_clk sysclk;

    tls_sys_clk_get(&sysclk);

    pwm_pin_config(channel,pin_number);

    tls_pwm_stop(channel);

    if (channel == 0)
        tls_pwm_isr_register(pwm_isr_callback0);
    else
        tls_pwm_isr_register(pwm_isr_callback4);

    tls_pwm_cap_init(channel, sysclk.apbclk * UNIT_MHZ / 256 / freq, DISABLE, WM_PWM_CAP_RISING_FALLING_EDGE_INT);
    tls_pwm_start(channel);
}


int drv_infrared_init()
{
    infrared = infrared_init();

    if(infrared == RT_NULL)
    {
        return -1;
    }

    infrared_send_init();
    infrared->send = infrared_send;

    pwm_capture_mode_int(4,38,10000); /* PA10 */

    return 0;
}
INIT_APP_EXPORT(drv_infrared_init);

#endif /* BSP_USING_INFRARED */
