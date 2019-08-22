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
#include <stdarg.h>
#include <string.h>
#include <rtthread.h>
#include "rtconfig.h"
#include "qcloud_iot_import.h"
#include "qcloud_iot_export.h"

#define DEBUG_DEV_INFO_USED

#ifdef DEBUG_DEV_INFO_USED

/* 产品ID, 与云端同步设备状态时需要  */
static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1]	 = PKG_USING_TENCENT_IOTHUB_PRODUCT_ID;

/* 设备名称, 与云端同步设备状态时需要 */
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]  = PKG_USING_TENCENT_IOTHUB_DEVICE_NAME;

/* 产品密钥, 与云端同步设备状态时需要  */
#ifdef PKG_USING_TENCENT_IOTHUB_DYNREG
static char sg_product_secret[MAX_SIZE_OF_PRODUCT_SECRET + 1]  = PKG_USING_TENCENT_IOTHUB_PRODUCT_SECRET;
#endif

#ifdef AUTH_MODE_CERT
/* 客户端证书文件名  非对称加密使用, TLS 证书认证方式*/
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1]      = "YOUR_DEVICE_NAME_cert.crt";
/* 客户端私钥文件名 非对称加密使用, TLS 证书认证方式*/
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_KEY_FILE_NAME + 1] = "YOUR_DEVICE_NAME_private.key";
#else
/* 设备密钥, TLS PSK认证方式*/
static char sg_device_secret[MAX_SIZE_OF_DEVICE_SERC + 1] = PKG_USING_TENCENT_IOTHUB_DEVICE_SECRET;
#endif

#endif


#define HAL_OS_LOG_MAXLEN   1024
static uint16_t g_mutex_count = 0;



void HAL_Printf(_IN_ const char *fmt, ...)
{
    va_list args;
	char log_buf[HAL_OS_LOG_MAXLEN];
    
    va_start(args, fmt);
    rt_vsnprintf(log_buf, HAL_OS_LOG_MAXLEN, fmt, args);
    va_end(args);
    printf("%s", log_buf);
}


int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}


int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}


void *HAL_MutexCreate(void)
{
	rt_mutex_t mutex;
	char mutexName[RT_NAME_MAX];

	memset(mutexName, 0, RT_NAME_MAX);
	HAL_Snprintf(mutexName, RT_NAME_MAX, "tmutex_%d", g_mutex_count++);	
	mutex = rt_mutex_create(mutexName, RT_IPC_FLAG_FIFO);
    if (NULL == mutex) {
		HAL_Printf("create mutex failed");

    }

    return mutex;


}

void HAL_MutexDestroy(_IN_ void *mutex)
{
	int err_num;

	err_num = rt_mutex_delete((rt_mutex_t)mutex);

    if (0 != err_num) 
	{
        HAL_Printf("destroy mutex failed");
    }
}

void HAL_MutexLock(_IN_ void *mutex)
{
	int err_num;
	
	err_num = rt_mutex_take((rt_mutex_t)mutex, RT_WAITING_FOREVER);

    if (0 != err_num)
	{
        HAL_Printf("lock mutex failed");
    }
}

void HAL_MutexUnlock(_IN_ void *mutex)
{
    int err_num;

	err_num = rt_mutex_take((rt_mutex_t)mutex, RT_WAITING_FOREVER);
	 
    if (0 != err_num)
	{
        HAL_Printf("lock mutex failed");
    }

}

void *HAL_Malloc(_IN_ uint32_t size)
{
   return rt_malloc(size);
}

void HAL_Free(_IN_ void *ptr)
{
    rt_free(ptr);
}

void HAL_SleepMs(_IN_ uint32_t ms)
{
    (void)rt_thread_delay(rt_tick_from_millisecond(ms));
}


int HAL_SetDevInfo(void *pdevInfo)
{
	int ret = QCLOUD_ERR_SUCCESS;;
	DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;
	

	if(NULL == pdevInfo){
		return QCLOUD_ERR_DEV_INFO;
	}
	
#ifdef DEBUG_DEV_INFO_USED
	memset(sg_product_id, '\0', MAX_SIZE_OF_PRODUCT_ID);	
	memset(sg_device_name, '\0', MAX_SIZE_OF_DEVICE_NAME);
	
	strncpy(sg_product_id, devInfo->product_id, MAX_SIZE_OF_PRODUCT_ID);
	strncpy(sg_device_name, devInfo->device_name, MAX_SIZE_OF_DEVICE_NAME);
	
#ifdef DEV_DYN_REG_ENABLED
	memset(sg_product_secret, '\0', MAX_SIZE_OF_PRODUCT_SECRET);
	strncpy(sg_product_secret, devInfo->product_secret, MAX_SIZE_OF_PRODUCT_SECRET);
#endif
	
#ifdef 	AUTH_MODE_CERT
	memset(sg_device_cert_file_name, '\0', MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	memset(sg_device_privatekey_file_name, '\0', MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
	
	strncpy(sg_device_cert_file_name, devInfo->devCertFileName, MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	strncpy(sg_device_privatekey_file_name, devInfo->devPrivateKeyFileName, MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
#else
	memset(sg_device_secret, '\0', MAX_SIZE_OF_DEVICE_SERC);
	strncpy(sg_device_secret, devInfo->devSerc, MAX_SIZE_OF_DEVICE_SERC);
#endif
		
#else
	 Log_e("HAL_SetDevInfo is not implement");
	 (void)devInfo; //eliminate compile warning

	 return QCLOUD_ERR_FAILURE;

#endif

	return ret;
}


int HAL_GetDevInfo(void *pdevInfo)
{
	int ret = QCLOUD_ERR_SUCCESS;
	DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;

	if(NULL == pdevInfo){
		return QCLOUD_ERR_DEV_INFO;
	}
	
	memset((char *)devInfo, '\0', sizeof(DeviceInfo));	
	
#ifdef DEBUG_DEV_INFO_USED	

	strncpy(devInfo->product_id, sg_product_id, MAX_SIZE_OF_PRODUCT_ID);
	strncpy(devInfo->device_name, sg_device_name, MAX_SIZE_OF_DEVICE_NAME);
	
#ifdef PKG_USING_TENCENT_IOTHUB_DYNREG
	memset(devInfo->product_secret, '\0', MAX_SIZE_OF_PRODUCT_SECRET);
	strncpy(devInfo->product_secret, sg_product_secret, MAX_SIZE_OF_PRODUCT_SECRET);
#endif	
	
#ifdef 	AUTH_MODE_CERT
	strncpy(devInfo->devCertFileName, sg_device_cert_file_name, MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	strncpy(devInfo->devPrivateKeyFileName, sg_device_privatekey_file_name, MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
#else
	strncpy(devInfo->devSerc, sg_device_secret, MAX_SIZE_OF_DEVICE_SERC);
#endif 

#else
   Log_e("HAL_GetDevInfo is not implement");

   return QCLOUD_ERR_FAILURE;
#endif

	return ret;
}


