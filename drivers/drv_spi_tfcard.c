/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2019-5-22      yangjie           the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_spi.h"
#include "spi_msd.h"
#include "board.h"

#ifdef BSP_USING_TF_CARD

static int rt_hw_spi0_tfcard(void)
{
    wm_spi_bus_attach_device(WM_SPI_BUS_NAME, "tf_spi", PIN_SD_CS);
    return msd_init("sd0", "tf_spi");
}
INIT_COMPONENT_EXPORT(rt_hw_spi0_tfcard);

#endif /*BSP_USING_TF_CARD*/
