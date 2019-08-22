import os
from building import *
import rtconfig

cwd  = GetCurrentDir()

src_base  = []

sample_mqtt_basic_src  = []
sample_mqtt_shadow_src  = []
sample_mqtt_door_src  = []
sample_air_shadow_src  = []
sample_coap_src  = []
sample_coap_door_src = []
sample_data_template_src = []
sample_light_data_template_src = []

CPPPATH = []
CPPDEFINES = []
LOCAL_CCFLAGS = ''


#include headfile
CPPPATH += [cwd + '/ports/rtthread/include']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/mqtt/include']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/shadow/include']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/coap/include']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/device/include']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/system/include']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/sdk-impl']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/sdk-impl/exports']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/utils/digest']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/utils/farra']
CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/utils/lite']

#Gen MQTT src file
if GetDepend(['PKG_USING_TENCENT_IOTHUB_MQTT']) or GetDepend(['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE']):
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/mqtt/src/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/device/src/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/sdk-impl/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/utils/digest/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/utils/farra/*.c')
	src_base += Glob('ports/rtthread/*.c')
	SrcRemove(src_base, 'qcloud-iot-sdk-embedded-c/src/utils/digest/utils_aes.c')
	SrcRemove(src_base, 'qcloud-iot-sdk-embedded-c/src/utils/farra/utils_httpc.c')
	SrcRemove(src_base, 'qcloud-iot-sdk-embedded-c/src/device/src/ca.c')	
	SrcRemove(src_base, 'ports/rtthread/HAL_UDP_rtthread.c')
	CPPDEFINES += ['MQTT_COMM_ENABLED', 'AUTH_MODE_KEY']

#Gen shadow src file
if GetDepend(['PKG_USING_TENCENT_IOTHUB_SHADOW']) or GetDepend(['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE']):
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/shadow/src/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/utils/lite/*.c')
	
#Gen COAP src file
if GetDepend(['PKG_USING_TENCENT_IOTHUB_COAP']):
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/coap/src/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/sdk-impl/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/device/src/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/utils/digest/*.c')
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/utils/farra/*.c')
	src_base += Glob('ports/rtthread/*.c')	
	SrcRemove(src_base, 'qcloud-iot-sdk-embedded-c/src/utils/digest/utils_aes.c')
	SrcRemove(src_base, 'qcloud-iot-sdk-embedded-c/src/utils/farra/utils_httpc.c')
	SrcRemove(src_base, 'qcloud-iot-sdk-embedded-c/src/device/src/ca.c')
	SrcRemove(src_base, 'ports/rtthread/HAL_UDP_rtthread.c')
	CPPDEFINES += ['COAP_COMM_ENABLED', 'AUTH_MODE_KEY']

#Gen event src file
if GetDepend(['PKG_USING_EVENT']):
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/event/src/*.c')
	CPPDEFINES += ['EVENT_POST_ENABLED']

#Err log upload used	
if GetDepend(['PKG_USING_TENCENT_IOTHUB_LOG_UPLOAD']):
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/system/src/*.c')
	CPPDEFINES += ['LOG_UPLOAD']
	
#TLS used
if GetDepend(['PKG_USING_TENCENT_IOTHUB_TLS']):
	src_base += Glob('qcloud-iot-sdk-embedded-c/src/device/src/ca.c')
	CPPDEFINES += ['MBEDTLS_CONFIG_FILE=<tc_tls_config.h>']
	
	if GetDepend(['PKG_USING_TENCENT_IOTHUB_MQTT']) or GetDepend(['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE']):
		src_base += Glob('ports/ssl/HAL_TLS_mbedtls.c')
		
	if GetDepend(['PKG_USING_TENCENT_IOTHUB_COAP']):
		src_base += Glob('ports/ssl/HAL_DTLS_mbedtls.c')	
else:
	CPPDEFINES += ['AUTH_WITH_NOTLS']
	
#DTLS used
#if GetDepend(['PKG_USING_TENCENT_IOTHUB_KIT_COAP_DTLS']):
#	src_base += Glob('ports/ssl/HAL_DTLS_mbedtls.c')

#Hub C-SDK core
group = DefineGroup('tencent-iothub', src_base, depend = ['PKG_USING_TENCENT_IOTHUB'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)

#MQTT Example
if GetDepend(['PKG_USING_TENCENT_IOTHUB_MQTT_BASIC']):
	sample_mqtt_basic_src += Glob('samples/iot_hub_platform/mqtt_sample.c')
	
group = DefineGroup('tc_sample_mqtt_basic', sample_mqtt_basic_src, depend = ['PKG_USING_TENCENT_IOTHUB_MQTT_BASIC'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)	

#Shadow Example
if GetDepend(['PKG_USING_TENCENT_IOTHUB_SHADOW']):
	sample_mqtt_shadow_src += Glob('samples/iot_hub_platform/shadow_sample.c')
	CPPPATH += [cwd + '/qcloud-iot-sdk-embedded-c/src/shadow/include']
	
group = DefineGroup('tc_sample_mqtt_shadow', sample_mqtt_shadow_src, depend = ['PKG_USING_TENCENT_IOTHUB_SHADOW'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)	

#MQTT door Example
if GetDepend(['PKG_USING_TENCENT_IOTHUB_DOOR_MQTT']):
	sample_mqtt_door_src += Glob('samples/iot_hub_platform/door_mqtt_sample.c')
	
group = DefineGroup('tc_sample_door_mqtt', sample_mqtt_door_src, depend = ['PKG_USING_TENCENT_IOTHUB_DOOR_MQTT'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)

#Shadow aircondition Example
if GetDepend(['PKG_USING_TENCENT_IOTHUB_AIRCON_SHADOW']):
	sample_air_shadow_src += Glob('samples/iot_hub_platform/aircond_shadow_sample.c')
	
group = DefineGroup('sample_air_shadow', sample_air_shadow_src, depend = ['PKG_USING_TENCENT_IOTHUB_AIRCON_SHADOW'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)

#COAP Example
if GetDepend(['PKG_USING_TENCENT_IOTHUB_COAP']):
	sample_coap_src += Glob('samples/iot_hub_platform/coap_sample.c')
	
group = DefineGroup('sample_coap', sample_coap_src, depend = ['PKG_USING_TENCENT_IOTHUB_COAP'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)

#COAP door Example	
if GetDepend(['PKG_USING_TENCENT_IOTHUB_DOOR_COAP']):
	sample_coap_door_src += Glob('samples/iot_hub_platform/door_coap_sample.c')
	
group = DefineGroup('sample_coap_door', sample_coap_door_src, depend = ['PKG_USING_TENCENT_IOTHUB_DOOR_COAP'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)
	
#Data Template Example
if GetDepend(['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE']):
	sample_data_template_src += Glob('samples/iot_explorer_platform/data_template_sample.c')
	
group = DefineGroup('sample_data_template', sample_data_template_src, depend = ['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)

#Data Template light Example
if GetDepend(['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE_LIGHT']):
	sample_light_data_template_src += Glob('samples/iot_explorer_platform/light_data_template_sample.c')
	
group = DefineGroup('sample_data_template_light', sample_light_data_template_src, depend = ['PKG_USING_TENCENT_IOTEXPLORER_DATA_TEMPLATE_LIGHT'], CPPPATH = CPPPATH, LOCAL_CCFLAGS = LOCAL_CCFLAGS, CPPDEFINES = CPPDEFINES)
		
  
Return('group')
