/**
 * @file    wm_watchdog.h
 *
 * @brief   watchdog Driver Module
 *
 * @author  dave
 *
 * Copyright (c) 2014 Winner Microelectronics Co., Ltd.
 */
#ifndef WM_WATCHDOG_H
#define WM_WATCHDOG_H

#include "wm_type_def.h"

/**
 * @defgroup Driver_APIs Driver APIs
 * @brief Driver APIs
 */

/**
 * @addtogroup Driver_APIs
 * @{
 */

/**
 * @defgroup WDG_Driver_APIs WDG Driver APIs
 * @brief WDG driver APIs
 */

/**
 * @addtogroup WDG_Driver_APIs
 * @{
 */

/**
 * @brief          This function is used to feed the dog.
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_clr(void);

/**
 * @brief          This function is used to init and start the watchdog.
 *
 * @param[in]      usec    microseconds
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_init(u32 usec);

/**
 * @brief          This function is used to reset the system.
 *
 * @param          None
 *
 * @return         None
 *
 * @note           None
 */
void tls_sys_reset(void);
/**
 * @brief          This function is used to start watchdog
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_start(void);
/**
 * @brief          This function is used to stop watchdog
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_stop(void);
/**
 * @brief          This function is used to set watchdog time
 *
 * @param[in]      usec    microseconds
 *
 * @return         None
 *
 * @note           None
 */
void tls_watchdog_set_timeout(u32 usec);
/**
 * @brief          This function is used to get watchdog timeout time
 *
 * @param[in]      None
 *
 * @return         u32 microseconds
 *
 * @note           None
 */
u32 tls_watchdog_get_timeout(void);
/**
 * @brief          This function is used to get watchdog left time
 *
 * @param[in]      none
 *
 * @return         u32  microseconds
 *
 * @note           None
 */
u32 tls_watchdog_get_timeleft(void);

/**
 * @}
 */

/**
 * @}
 */

#endif /* WM_WATCHDOG_H */

