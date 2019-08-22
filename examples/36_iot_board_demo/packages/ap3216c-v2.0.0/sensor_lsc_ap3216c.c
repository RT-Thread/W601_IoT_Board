/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-31     yangjie      the first version
 */

#include "sensor_lsc_ap3216c.h"
#include "ap3216c.h"

#define DBG_TAG "sensor.lsc.ap3216c"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SENSOR_ALS_RANGE_MAX  AP3216C_ALS_RANGE_20661
#define SENSOR_ALS_RANGE_MIN  (0)
#define SENSOR_PS_RANGE_MAX   (1023)
#define SENSOR_PS_RANGE_MIN   (0)

static rt_size_t _ap3216c_polling_get_data(rt_sensor_t sensor, struct rt_sensor_data *data)
{
    float light_x10;
    struct ap3216c_device *als_ps_dev = sensor->parent.user_data;

    if (sensor->info.type == RT_SENSOR_CLASS_LIGHT)
    {
        light_x10 = 10 * ap3216c_read_ambient_light(als_ps_dev);
        data->data.light = (rt_int32_t)light_x10;
        data->timestamp = rt_sensor_get_ts();
    }
    else if (sensor->info.type == RT_SENSOR_CLASS_PROXIMITY)
    {
        data->data.proximity = (rt_int32_t)(ap3216c_read_ps_data(als_ps_dev));
        data->timestamp = rt_sensor_get_ts();
    }
    return 1;
}

static rt_size_t ap3216c_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    RT_ASSERT(buf);

    if (sensor->config.mode == RT_SENSOR_MODE_POLLING)
    {
        return _ap3216c_polling_get_data(sensor, buf);
    }

    return 0;
}


rt_err_t _ap3216c_set_range(struct rt_sensor_device *sensor, uint8_t range)
{
    rt_err_t result = RT_EOK;
    
    struct ap3216c_device *als_ps_dev = sensor->parent.user_data;
    
    if(sensor->info.type == RT_SENSOR_CLASS_LIGHT)
    {
        ap3216c_set_param(als_ps_dev, AP3216C_ALS_RANGE, range);
    }
    if(sensor->info.type == RT_SENSOR_CLASS_PROXIMITY)
    {
        ap3216c_set_param(als_ps_dev, AP3216C_PS_GAIN, range);
    }
    
    return result;
}

rt_err_t _ap3216c_set_mode(struct rt_sensor_device *sensor, uint8_t mode)
{
    rt_err_t result = RT_EOK; 

    if (mode == RT_SENSOR_MODE_POLLING)
    {
        LOG_D("set mode to POLLING");
    }
    else if (mode == RT_SENSOR_MODE_INT)
    {
        LOG_D("set mode to INTERRUPT");
    }
    else
    {
        LOG_D("Unsupported mode, code is %d", mode);
        return -RT_ERROR;
    }

    return result;
}
static rt_err_t ap3216c_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;

    switch (cmd)
    {
    case RT_SENSOR_CTRL_SET_RANGE:
        result = _ap3216c_set_range(sensor, (rt_uint32_t)args);
        break;
    case RT_SENSOR_CTRL_SET_MODE:
        result = _ap3216c_set_mode(sensor, (rt_uint32_t)args);
        break;
    default:
        return -RT_ERROR;
    }

    return result;
}

static struct rt_sensor_ops sensor_ops =
{
    ap3216c_fetch_data,
    ap3216c_control
};

int rt_hw_ap3216c_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor_als = RT_NULL, sensor_ps = RT_NULL;
    struct ap3216c_device *als_ps_dev;

#ifdef PKG_USING_AP3216C
    
    als_ps_dev = ap3216c_init(cfg->intf.dev_name);
    if (als_ps_dev == RT_NULL)
    {
        return -RT_ERROR;
    }

    /* temperature sensor register */
    sensor_als = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor_als == RT_NULL)
        return -1;

    sensor_als->info.type       = RT_SENSOR_CLASS_LIGHT;
    sensor_als->info.vendor     = RT_SENSOR_VENDOR_UNKNOWN;
    sensor_als->info.model      = "ap3216c";
    sensor_als->info.unit       = RT_SENSOR_UNIT_LUX;
    sensor_als->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor_als->info.range_max  = SENSOR_ALS_RANGE_MAX;
    sensor_als->info.range_min  = SENSOR_ALS_RANGE_MIN;
    sensor_als->info.period_min = 5;

    rt_memcpy(&sensor_als->config, cfg, sizeof(struct rt_sensor_config));
    sensor_als->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor_als, name, RT_DEVICE_FLAG_RDWR, als_ps_dev);    
    if (result != RT_EOK)
    {
        LOG_E("device register err code: %d", result);
        goto __exit;
    }

    /* humidity sensor register */
    sensor_ps = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor_ps == RT_NULL)
        return -1;

    sensor_ps->info.type       = RT_SENSOR_CLASS_PROXIMITY;
    sensor_ps->info.vendor     = RT_SENSOR_VENDOR_UNKNOWN;
    sensor_ps->info.model      = "ap3216c";
    sensor_ps->info.unit       = RT_SENSOR_UNIT_CM;
    sensor_ps->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor_ps->info.range_max  = SENSOR_PS_RANGE_MAX;
    sensor_ps->info.range_min  = SENSOR_PS_RANGE_MIN;
    sensor_ps->info.period_min = 5;

    rt_memcpy(&sensor_ps->config, cfg, sizeof(struct rt_sensor_config));
    sensor_ps->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor_ps, name, RT_DEVICE_FLAG_RDWR, als_ps_dev);
    if (result != RT_EOK)
    {
        LOG_E("device register err code: %d", result);
        goto __exit;
    }

#endif

    return RT_EOK;

__exit:
    if (sensor_als)
        rt_free(sensor_als);
    if (sensor_ps)
        rt_free(sensor_ps);
    if (als_ps_dev)
        ap3216c_deinit(als_ps_dev);
    return -RT_ERROR;
}
