/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

#include "rtthread.h"


#define MQTT_SHADOW_THREAD_STACK_SIZE 	6144
static int running_state = 0;

#ifdef AUTH_MODE_CERT
    static char sg_cert_file[PATH_MAX + 1];      //客户端证书全路径
    static char sg_key_file[PATH_MAX + 1];       //客户端密钥全路径
#endif

static DeviceInfo sg_devInfo;

static char sg_shadow_update_buffer[200];
size_t sg_shadow_update_buffersize = sizeof(sg_shadow_update_buffer) / sizeof(sg_shadow_update_buffer[0]);

static DeviceProperty sg_shadow_property;
static int sg_current_update_count = 0;
static bool sg_delta_arrived = false;

void OnDeltaCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty) {
	int rc = IOT_Shadow_JSON_ConstructDesireAllNull(pClient, sg_shadow_update_buffer, sg_shadow_update_buffersize);

	if (rc == QCLOUD_ERR_SUCCESS) {
		sg_delta_arrived = true;
	}
	else {
		Log_e("construct desire failed, err: %d", rc);
	}
}

void OnShadowUpdateCallback(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *pUserdata) {
	Log_i("recv shadow update response, response ack: %d", requestAck);
}

static int _setup_connect_init_params(ShadowInitParams* initParams)
{
	int ret;
	
	ret = HAL_GetDevInfo((void *)&sg_devInfo);	
	if(QCLOUD_ERR_SUCCESS != ret){
		return ret;
	}
	
	initParams->device_name = sg_devInfo.device_name;
	initParams->product_id = sg_devInfo.product_id;

#ifdef AUTH_MODE_CERT
	/* 使用非对称加密*/
	char certs_dir[PATH_MAX + 1] = "certs";
	char current_path[PATH_MAX + 1];
	char *cwd = getcwd(current_path, sizeof(current_path));
	if (cwd == NULL)
	{
		Log_e("getcwd return NULL");
		return QCLOUD_ERR_FAILURE;
	}
	sprintf(sg_cert_file, "%s/%s/%s", current_path, certs_dir, sg_devInfo.devCertFileName);
	sprintf(sg_key_file, "%s/%s/%s", current_path, certs_dir, sg_devInfo.devPrivateKeyFileName);

	initParams->cert_file = sg_cert_file;
	initParams->key_file = sg_key_file;
#else
	initParams->device_secret = sg_devInfo.devSerc;
#endif

	initParams->command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
	initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
	initParams->auto_connect_enable = 1;

    return QCLOUD_ERR_SUCCESS;
}


static void mqtt_shadow_thread(void)
{
	int rc = QCLOUD_ERR_FAILURE;

	void* shadow_client = NULL;
	
	//init connection
	ShadowInitParams init_params = DEFAULT_SHAWDOW_INIT_PARAMS;		
    rc = _setup_connect_init_params(&init_params);
	if (rc != QCLOUD_ERR_SUCCESS) {
		Log_e("init params err,rc=%d", rc);
		return;
	}

	shadow_client = IOT_Shadow_Construct(&init_params);
	if (shadow_client == NULL) {
		Log_e("shadow client constructed failed.");
		return;
	}

	//注册delta属性
	sg_shadow_property.key = "updateCount";
	sg_shadow_property.data = &sg_current_update_count;
	sg_shadow_property.type = JINT32;
	rc = IOT_Shadow_Register_Property(shadow_client, &sg_shadow_property, OnDeltaCallback);
	if (rc != QCLOUD_ERR_SUCCESS) {
		rc = IOT_Shadow_Destroy(shadow_client);
		Log_e("register device shadow property failed, err: %d", rc);
		return;
	}

	//进行Shdaow Update操作的之前，最后进行一次同步操作，否则可能本机上shadow version和云上不一致导致Shadow Update操作失败
	rc = IOT_Shadow_Get_Sync(shadow_client, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	if (rc != QCLOUD_ERR_SUCCESS) {
		Log_e("get device shadow failed, err: %d", rc);
		return;
	}

	running_state = 1;
	while (IOT_Shadow_IsConnected(shadow_client) || QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT == rc ||
			QCLOUD_ERR_MQTT_RECONNECTED == rc || QCLOUD_ERR_SUCCESS == rc ) {
			
		if(0 == running_state){
			break;
		}	

		rc = IOT_Shadow_Yield(shadow_client, 2000);

		if (QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT == rc) {
			HAL_SleepMs(1000);
			continue;
		}
		else if (rc != QCLOUD_ERR_SUCCESS && rc != QCLOUD_ERR_MQTT_RECONNECTED) {
			Log_e("exit with error: %d", rc);
			return;
		}

		if (sg_delta_arrived) {
			rc = IOT_Shadow_Update_Sync(shadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
			sg_delta_arrived = false;
			if (rc == QCLOUD_ERR_SUCCESS) 
				Log_i("shadow update success");
		}

		IOT_Shadow_JSON_ConstructReport(shadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, 1, &sg_shadow_property);
		rc = IOT_Shadow_Update(shadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, OnShadowUpdateCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		sg_current_update_count++;

		// sleep for some time in seconds
		HAL_SleepMs(1000);
	}
	rc = IOT_Shadow_Destroy(shadow_client);
	running_state = 0;
	Log_e("Something goes wrong or stoped"); 

	return;
}


int tc_shadow_example(int argc, char **argv)
{
	rt_thread_t tid;
    int stack_size = MQTT_SHADOW_THREAD_STACK_SIZE;

	IOT_Log_Set_Level(DEBUG);
	if (2 == argc)
	{
		if (!strcmp("start", argv[1]))
		{
			if (1 == running_state)
			{
				Log_d("tc_shadow_example is already running\n");
				return 0;
			}			
		}
		else if (!strcmp("stop", argv[1]))
		{
			if (0 == running_state)
			{
				Log_d("tc_shadow_example is already stopped\n");
				return 0;
			}
			running_state = 0;
			return 0;
		}
		else
		{
			Log_d("Usage: tc_shadow_example start/stop");
			return 0;			  
		}
	}
	else
	{
		Log_e("Para err, usage: tc_shadow_example start/stop");
		return 0;
	}
	  
	tid = rt_thread_create("mqtt_shadow", (void (*)(void *))mqtt_shadow_thread, 
							NULL, stack_size, RT_THREAD_PRIORITY_MAX / 2 - 1, 10);  

    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }

    return 0;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(tc_shadow_example, startup mqtt shadow example);
#endif

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(tc_shadow_example, startup mqtt shadow example);
#endif

