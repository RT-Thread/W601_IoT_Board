/**
 * @file    wm_watchdog.c
 *
 * @brief   watchdog Driver Module
 *
 * @author  kevin
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#include "wm_debug.h"
#include "wm_regs.h"
#include "wm_irq.h"
#include "wm_cpu.h"
#include "wm_watchdog.h"

// void WDG_IRQHandler(void)
// {
    // wm_printf("WDG IRQ\n");
// }

/**
 * @brief          This function is used to clear watchdog
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_clr(void)
{
    tls_reg_write32(HR_WDG_INT_CLR, 0x01);
}

/**
 * @brief          This function is used to init watchdog
 *
 * @param[in]      usec    microseconds
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_init(u32 usec)
{
    tls_sys_clk sysclk;

    tls_sys_clk_get(&sysclk);
    tls_irq_enable(WATCHDOG_INT);

    tls_reg_write32(HR_WDG_LOAD_VALUE, sysclk.apbclk * usec); /* 40M dominant frequency: 40 * 10^6 * (usec / 10^6) */
    tls_reg_write32(HR_WDG_CTRL, 0x3);						  /* enable irq & reset */
}
/**
 * @brief          This function is used to start watchdog
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_start(void)
{
    tls_reg_write32(HR_WDG_CTRL, 0x3);
}

/**
 * @brief          This function is used to stop watchdog
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_stop(void)
{
    tls_reg_write32(HR_WDG_CTRL, 0x0);
}
/**
 * @brief          This function is used to set watchdog time
 *
 * @param[in]      usec    microseconds
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_set_timeout(u32 usec)
{
    tls_sys_clk sysclk;

    tls_reg_write32(HR_WDG_INT_CLR, 0x01);
    tls_irq_disable(WATCHDOG_INT);
    tls_reg_write32(HR_WDG_CTRL, 0);

    tls_sys_clk_get(&sysclk);
    tls_irq_enable(WATCHDOG_INT);
    tls_reg_write32(HR_WDG_LOAD_VALUE, sysclk.apbclk * usec); /* 40M dominant frequency: 40 * 10^6 * (usec / 10^6) */
    tls_reg_write32(HR_WDG_CTRL, 0x3);
}
/**
 * @brief          This function is used to get watchdog timeout time
 *
 * @param[in]      None
 *
 * @return         u32 microseconds
 *
 * @note           None
 */
u32 tls_watchdog_get_timeout(void)
{
    u32 timeout_tick = 0;
    tls_sys_clk sysclk;

    tls_sys_clk_get(&sysclk);
    timeout_tick = tls_reg_read32(HR_WDG_LOAD_VALUE);

    return (u32)(timeout_tick / sysclk.apbclk);
}
/**
 * @brief          This function is used to get watchdog left time
 *
 * @param[in]      none
 *
 * @return         u32  microseconds
 *
 * @note           None
 */
u32 tls_watchdog_get_timeleft(void)
{
    u32 timeout_tick = 0;
    tls_sys_clk sysclk;

    tls_sys_clk_get(&sysclk);
    timeout_tick = tls_reg_read32(HR_WDG_CUR_VALUE);

    return (u32)(timeout_tick / sysclk.apbclk);
}
/**
 * @brief          This function is used to reset system
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
void tls_sys_reset(void)
{
    tls_reg_write32(HR_WDG_LOCK, 0x1ACCE551);
    tls_reg_write32(HR_WDG_LOAD_VALUE, 0x100);
    tls_reg_write32(HR_WDG_CTRL, 0x3);
    tls_reg_write32(HR_WDG_LOCK, 1);
}
