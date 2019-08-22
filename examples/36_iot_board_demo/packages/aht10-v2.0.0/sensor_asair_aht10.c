/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-08     yangjie      the first version
 */
 
#include "sensor_asair_aht10.h"

#define DBG_TAG "sensor.asair.aht10"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SENSOR_TEMP_RANGE_MAX (85)
#define SENSOR_TEMP_RANGE_MIN (-40)
#define SENSOR_HUMI_RANGE_MAX (100)
#define SENSOR_HUMI_RANGE_MIN (0)

static struct aht10_device *temp_humi_dev;

static rt_err_t _aht10_init(struct rt_sensor_intf *intf)
{
    temp_humi_dev = aht10_init(intf->dev_name);

    if (temp_humi_dev == RT_NULL)
    {
        return -RT_ERROR;
    }
    
    return RT_EOK;
}

static rt_size_t _aht10_polling_get_data(rt_sensor_t sensor, struct rt_sensor_data *data)
{
    float temperature_x10, humidity_x10;
    
    if (sensor->info.type == RT_SENSOR_CLASS_TEMP)
    {
        temperature_x10 = 10 * aht10_read_temperature(temp_humi_dev);
        data->data.temp = (rt_int32_t)temperature_x10;
        data->timestamp = rt_sensor_get_ts();
    }    
    else if (sensor->info.type == RT_SENSOR_CLASS_HUMI)
    {
        humidity_x10    = 10 * aht10_read_humidity(temp_humi_dev);
        data->data.humi = (rt_int32_t)humidity_x10;
        data->timestamp = rt_sensor_get_ts();
    }
    return 1;
}

static rt_size_t aht10_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    RT_ASSERT(buf);

    if (sensor->config.mode == RT_SENSOR_MODE_POLLING)
    {
        return _aht10_polling_get_data(sensor, buf);
    }
    else
        return 0;
}

static rt_err_t aht10_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;

    return result;
}

static struct rt_sensor_ops sensor_ops =
{
    aht10_fetch_data,
    aht10_control
};

int rt_hw_aht10_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor_temp = RT_NULL, sensor_humi = RT_NULL;
    
#ifdef PKG_USING_AHT10   
    
     /* temperature sensor register */
    sensor_temp = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor_temp == RT_NULL)
        return -1;

    sensor_temp->info.type       = RT_SENSOR_CLASS_TEMP;
    sensor_temp->info.vendor     = RT_SENSOR_VENDOR_UNKNOWN;
    sensor_temp->info.model      = "aht10";
    sensor_temp->info.unit       = RT_SENSOR_UNIT_DCELSIUS;
    sensor_temp->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor_temp->info.range_max  = SENSOR_TEMP_RANGE_MAX;
    sensor_temp->info.range_min  = SENSOR_TEMP_RANGE_MIN;
    sensor_temp->info.period_min = 5;

    rt_memcpy(&sensor_temp->config, cfg, sizeof(struct rt_sensor_config));
    sensor_temp->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor_temp, name, RT_DEVICE_FLAG_RDONLY, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("device register err code: %d", result);
        goto __exit;
    }
    
    /* humidity sensor register */
    sensor_humi = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor_humi == RT_NULL)
        return -1;

    sensor_humi->info.type       = RT_SENSOR_CLASS_HUMI;
    sensor_humi->info.vendor     = RT_SENSOR_VENDOR_UNKNOWN;
    sensor_humi->info.model      = "aht10";
    sensor_humi->info.unit       = RT_SENSOR_UNIT_PERMILLAGE;
    sensor_humi->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor_humi->info.range_max  = SENSOR_HUMI_RANGE_MAX;
    sensor_humi->info.range_min  = SENSOR_HUMI_RANGE_MIN;
    sensor_humi->info.period_min = 5;

    rt_memcpy(&sensor_humi->config, cfg, sizeof(struct rt_sensor_config));
    sensor_humi->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor_humi, name, RT_DEVICE_FLAG_RDONLY, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("device register err code: %d", result);
        goto __exit;
    }
    
#endif
    
    _aht10_init(&cfg->intf);
    return RT_EOK;
    
__exit:
    if (sensor_temp)
        rt_free(sensor_temp);
    if (sensor_humi)
        rt_free(sensor_humi);
    if (temp_humi_dev)
        aht10_deinit(temp_humi_dev);
    return -RT_ERROR;     
}
