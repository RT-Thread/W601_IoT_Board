/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-08     yangjie      the first version
 */

#ifndef SENSOR_ASSIR_AHT10_H__
#define SENSOR_ASSIR_AHT10_H__

#include "sensor.h"
#include "aht10.h"

#define AHT10_I2C_ADDR 0x38

int rt_hw_aht10_init(const char *name, struct rt_sensor_config *cfg);

#endif
