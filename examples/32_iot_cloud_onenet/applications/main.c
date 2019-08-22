/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-01     ZYLX         first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdlib.h>
#include <onenet.h>
#include <string.h>
#include <easyflash.h>
#include <fal.h>
#include "wifi_config.h"
#include "sensor_lsc_ap3216c.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define LED_PIN PIN_LED_R
#define NUMBER_OF_UPLOADS 100

#define ALS_DEV       "li_ap321"

static rt_device_t als_dev;

static void onenet_cmd_rsp_cb(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size);

int main(void)
{
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    fal_init();
    easyflash_init();

    /* 配置 wifi 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);

    /* 初始化 wlan 自动连接 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);
	
    /* 查找光强传感器 */
    als_dev = rt_device_find(ALS_DEV);
    if (als_dev == RT_NULL)
    {
        LOG_E("can not find ALS device: %s", ALS_DEV);
        return -RT_ERROR;
    }
    /* 打开光强传感器 */
    if (rt_device_open(als_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
    {
        LOG_E("open ALS device failed!");
        return -RT_ERROR;
    }

    return 0;
}

static void onenet_upload_entry(void *parameter)
{
    struct rt_sensor_data als_dev_data = { 0 };
    int i = 0;

    /* 往 light 数据流上传环境光数据 */
    for (i = 0; i < NUMBER_OF_UPLOADS; i++)
    {
        if (als_dev)
        {
            rt_device_read(als_dev, 0, &als_dev_data, 1);
        }

        if (onenet_mqtt_upload_digit("light", als_dev_data.data.light / 10) < 0)
        {
            LOG_E("upload has an error, stop uploading");
            break;
        }
        else
        {
            LOG_D("buffer : {\"light\":%d}", als_dev_data.data.light / 10);
        }

        rt_thread_mdelay(5 * 1000);
    }
}

void onenet_upload_cycle(void)
{
    rt_thread_t tid;
	
    /* 设置 onenet 回调响应函数 */
    onenet_set_cmd_rsp_cb(onenet_cmd_rsp_cb);

    /* 传创建线程 */
    tid = rt_thread_create("onenet_send",
                           onenet_upload_entry,
                           RT_NULL,
                           2 * 1024,
                           RT_THREAD_PRIORITY_MAX / 3 - 1,
                           5);
    if (tid)
    {
        rt_thread_startup(tid);
    }
}
MSH_CMD_EXPORT(onenet_upload_cycle, upload data to onenet);

rt_err_t onenet_port_save_device_info(char *dev_id, char *api_key)
{
    EfErrCode err = EF_NO_ERR;

    /* 保存设备 ID */
    err = ef_set_and_save_env("dev_id", dev_id);
    if (err != EF_NO_ERR)
    {
        LOG_E("save device info(dev_id : %s) failed!", dev_id);
        return -RT_ERROR;
    }

    /* 保存设备 api_key */
    err = ef_set_and_save_env("api_key", api_key);
    if (err != EF_NO_ERR)
    {
        LOG_E("save device info(api_key : %s) failed!", api_key);
        return -RT_ERROR;
    }

    /* 保存环境变量：已经注册 */
    err = ef_set_and_save_env("already_register", "1");
    if (err != EF_NO_ERR)
    {
        LOG_E("save already_register failed!");
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t onenet_port_get_register_info(char *dev_name, char *auth_info)
{
    rt_uint32_t udid[2] = {0}; /* 唯一设备标识 */
    EfErrCode err = EF_NO_ERR;

    /* 获得 MAC 地址 */
    if (rt_wlan_get_mac((rt_uint8_t *)udid) != RT_EOK)
    {
        LOG_E("get mac addr err!! exit");
        return -RT_ERROR;
    }

    /* 设置设备名和鉴权信息 */
    rt_snprintf(dev_name, ONENET_INFO_AUTH_LEN, "%d%d", udid[0], udid[1]);
    rt_snprintf(auth_info, ONENET_INFO_AUTH_LEN, "%d%d", udid[0], udid[1]);

    /* 保存设备鉴权信息 */
    err = ef_set_and_save_env("auth_info", auth_info);
    if (err != EF_NO_ERR)
    {
        LOG_E("save auth_info failed!");
        return -RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t onenet_port_get_device_info(char *dev_id, char *api_key, char *auth_info)
{
    char *info = RT_NULL;

    /* 获取设备 ID */
    info = ef_get_env("dev_id");
    if (info == RT_NULL)
    {
        LOG_E("read dev_id failed!");
        return -RT_ERROR;
    }
    else
    {
        rt_snprintf(dev_id, ONENET_INFO_AUTH_LEN, "%s", info);
    }

    /* 获取 api_key */
    info = ef_get_env("api_key");
    if (info == RT_NULL)
    {
        LOG_E("read api_key failed!");
        return -RT_ERROR;
    }
    else
    {
        rt_snprintf(api_key, ONENET_INFO_AUTH_LEN, "%s", info);
    }

    /* 获取设备鉴权信息 */
    info = ef_get_env("auth_info");
    if (info == RT_NULL)
    {
        LOG_E("read auth_info failed!");
        return -RT_ERROR;
    }
    else
    {
        rt_snprintf(auth_info, ONENET_INFO_AUTH_LEN, "%s", info);

    }

    return RT_EOK;
}

rt_bool_t onenet_port_is_registed(void)
{
    char *already_register = RT_NULL;

    /* 检查设备是否已经注册 */
    already_register = ef_get_env("already_register");
    if (already_register == RT_NULL)
    {
        return RT_FALSE;
    }

    return already_register[0] == '1' ? RT_TRUE : RT_FALSE;
}

static void onenet_cmd_rsp_cb(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    char res_buf[20] = {0};

    LOG_D("recv data is %.*s", recv_size, recv_data);

    /* 命令匹配 */
    if (rt_strncmp((const char *)recv_data, "ledon", 5) == 0)
    {
        /* 开灯 */
        rt_pin_write(LED_PIN, PIN_LOW);

        rt_snprintf(res_buf, sizeof(res_buf), "led is on");

        LOG_D("led is on");
    }
    else if (rt_strncmp((const char *) recv_data, "ledoff", 6) == 0)
    {
        /* 关灯 */
        rt_pin_write(LED_PIN, PIN_HIGH);

        rt_snprintf(res_buf, sizeof(res_buf), "led is off");

        LOG_D("led is off");
    }

    /* 开发者必须使用 ONENET_MALLOC 为响应数据申请内存 */
    *resp_data = (uint8_t *)ONENET_MALLOC(strlen(res_buf) + 1);

    strncpy((char*)resp_data, res_buf, strlen(res_buf));

    *resp_size = strlen(res_buf);
}
