/*
 * File      : rt_cld_mqtt.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-06     chenyong     the first version
 */

#ifndef _RT_OTA_MQTT_H_
#define _RT_OTA_MQTT_H_

#define CLD_INITIAL_VERSION            "0.0"

/* OTA upgrade mode */
enum CLD_UPGRADE_MODE
{
    CLD_UPGRADE_OPTIONAL = 1,
    CLD_UPGRADE_MANDATORY,
    CLD_UPGRADE_SILENCE,
};

enum CLD_MQTT_DP
{
    CLD_DP_RESTART = 1000,

    CLD_DP_OTA_GET_INFO = 1010,
    CLD_DP_OTA_UP_INFO,
    CLD_DP_OTA_GET_TASK,
    CLD_DP_OTA_UP_UPGRADE,

    CLD_DP_LOG_CONTROL = 1020,
    CLD_DP_LOG_UPLOAD_STATE,
    CLD_DP_LOG_GET_INFO,

    CLD_DP_TCP_SHELL = 1025,
};

#define CLD_SUBT_MAX_LEN               128
#define CLD_PUBT_MAX_LEN               128
#define CLD_SHELL_KEY_MAX_LEN          (48 + 1)
#define CLD_LOG_KEY_MAX_LEN            (48 + 1)

void rt_cld_mqtt_close(void);
int rt_cld_mqtt_init(void);

#endif /* __CLOUD_MQ_CLIENT_H__ */




