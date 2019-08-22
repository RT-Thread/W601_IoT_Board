##  Tencent IOT SDK for rt-thread Package 
## 1 介绍

Tencent IOT SDK for rt-thread Package 是基于[腾讯云设备端C-SDK](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c.git)在RThread环境开发的软件包，腾讯物联网设备端 C-SDK 依靠安全且性能强大的数据通道，为物联网领域开发人员提供终端和云端的双向通信能力。C-SDK V3.0.0版本以后同时支持腾讯的现有的两个物联网平台，[物联网通信平台](https://cloud.tencent.com/product/iothub)和[物联网开发平台](https://cloud.tencent.com/product/iotexplorer)。

### 1.1 SDK架构图
![sdk-architecture](https://main.qcloudimg.com/raw/fb3ce5f898ba604e47a396b0dd5dbf8e.jpg)

### 1.2 目录结构

| 名称            | 说明 |
| ----            | ---- |
| docs            | 文档目录 |
| qcloud-iot-sdk-embedded-c         | 腾讯物联网设备端C-SDK |
| ports           | 移植文件目录 |
| samples         | 示例目录 |
|  ├─iot_hub_platform         | 物联网通信平台示例 |
|  ├─iot_explorer_platform         | 物联网开发平台示例 |
| LICENSE         | 许可证文件 |
| README.md       | 软件包使用说明 |
| SConscript      | RT-Thread 默认的构建脚本 |

### 1.3 许可证

沿用`qcloud-iot-sdk-embedded-c`许可协议Apache 2.0。

## 2 软件包使用
### 2.1 menuconfig配置
- RT-Thread env开发工具中使用 `menuconfig` 使能 `tencent-iothub` 软件包，选择开发平台，配置产品及设备信息，并根据产品需求选择合适的应用示例修改新增业务逻辑，也可新增例程编写新的业务逻辑。

```
      --- tencent-iothub:  Tencent Cloud SDK for IoT platform             
                   Select Tencent IOT platform (Iot_hub platform)  --->   
             (03UKNYBUZG) Config Product Id                               
             (general_sdev1) Config Device Name                           
             (VI04Eh4N8VgM29U/dnu9cQ==) Config Device Secret              
             [ ]   Enable dynamic register                                
             [ ]   Enable err log upload                                  
             [ ]   Enable TLS/DTLS                                        
             [*]   Enable MQTT                                            
                     Enable shadow (ONLY MQTT)  --->                      
             [ ]   Enable COAP                                            
             [ ]   Enable SCENARIOS                                       
                   Version (latest)  --->
```

- 选项说明

`Select Tencent IOT platform`：选择物联网通信平台（Iot_hub）还是物联网开发平台（Iot_explorer）。

`Config Product Id`：配置产品ID，平台创建生成。

`Config Device Name`：配置设备名，平台创建生成。

`Config Device Secret`：配置设备密钥，平台创建生成，考虑到嵌入式设备大多没有文件系统，暂时没有支持证书设备配置。

`Enable dynamic register`：是否使能[设备动态注册](https://cloud.tencent.com/document/product/634)，若使能，则需要配置产品密钥，解决一型一密的场景。

`Enable err log upload`：是否使能错误日志上传云端。

`Enable TLS/DTLS`： 是否使能TLS，若使能，则会关联选中mbedTLS软件包。

- 物联网通信平台配置：

`Enable MQTT`：通信协议是否选用MQTT。

 ----`Enable shadow`：选用MQTT协议的前提下是否选用[影子协议](https://cloud.tencent.com/document/product/634/11918)。
 
`Enable COAP`：通信协议是否选用COAP。

`Enable SCENARIOS`：是否使能场景示例，若使能，可以选择对应通信协议下的场景示例。

`Version`：软件包版本选择

- 物联网开发平台配置：

`Enable data template`：是否使能[数据模板](https://cloud.tencent.com/document/product/1081/34916)。

 ----`Enable event`：选用数据模板的前提下是否使能事件功能。
 
`Enable SCENARIOS`：是否使能场景示例，若使能，可选择数据模板的智能灯场景示例。

`Version`：软件包版本选择

- 特别说明：使能TLS/DTLS后，会关联选用mbedTLS软件包，但会指定配置头文件为 `tc_tls_config.h`而非软件包的配置头文件`tls_config.h`， `tc_tls_config.h`进一步优化了资源，主要是对[非证书设备认证]()裁剪了X509证书。
>! 如果编译报错，请在`tls_client.h`、`tls_client.c`、`proto_mbedtls.c`这三个文件用到X509证书相关的地方增加编译宏 `MBEDTLS_X509_CRT_PARSE_C`隔离。

- 相关链接
[物联网开发平台SDK说明文档](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c/blob/master/docs/物联网开发平台.md)
 [物联网通信平台SDK说明文档](https://github.com/tencentyun/qcloud-iot-sdk-embedded-c/blob/master/docs/物联网通信平台.md)

- 使用 `pkgs --update` 命令下载软件包

#### 2.2 创建可订阅可发布的Topic

创建的设备默认只有两个topic，一个只有发布权限，一个只有订阅权限。配合MQTT示例，需要在控制台对应产品下创建可订阅发布的data topic。
进入控制台产品设置页面, 点击权限列表，再点击**添加Topic权限**。

![](https://main.qcloudimg.com/raw/65a2d1b7251de37ce1ca2ba334733c57.png)

在弹窗中输入 data, 并设置操作权限为**发布和订阅**，点击创建。

![](https://main.qcloudimg.com/raw/f429b32b12e3cb0cf319b1efe11ccceb.png)

随后将会创建出 productID/\${deviceName}/data 的 Topic，在产品页面的权限列表中可以查看该产品的所有权限。

### 2.3 编译及运行
1. 使用命令 scons --target=xxx 输出对应的工程，编译 

2. 执行示例程序：

### 2.4 运行demo程序
系统启动后，在 MSH 中使用命令执行：

#### 2.4.1  MQTT+TLS示例：
- 示例说明：该示例展示了设备和[物联网通信平台](https://cloud.tencent.com/product/iothub)的MQTT通信示例，包括MQTT连接、主题订阅、消息推送、消息收取回调处理，使能TLS。 

- 配置选项
```
             --- tencent-iothub:  Tencent Cloud SDK for IoT platform          
                     Select Tencent IOT platform (Iot_hub platform)  --->     
               (03UKNYBUZG) Config Product Id                                 
               (general_sdev1) Config Device Name                             
               (VI04Eh4N8VgM29U/dnu9cQ==) Config Device Secret                
               [ ]   Enable dynamic register                                  
               [ ]   Enable err log upload                                    
               [*]   Enable TLS/DTLS                                          
               [*]   Enable MQTT                                              
                       Enable shadow (ONLY MQTT)  --->                        
               [ ]   Enable COAP                                              
               [ ]   Enable SCENARIOS                                         
                     Version (latest)  --->
```

- 运行示例

```
  \ | /
- RT -     Thread Operating System
 / | \     4.0.2 build May 28 2019
 2006 - 2019 Copyright by rt-thread team
[I/sal.skt] Socket Abstraction Layer initialize success.
[I/at.clnt] AT client(V1.2.0) on device uart4 initialize success.
[I/at.esp8266] ESP8266 WIFI is disconnect.
[I/at.esp8266] ESP8266 WIFI is connected.
[I/at.esp8266] AT network initialize success!
msh />[E/at.clnt] execute command (AT+CIPDNS_CUR?) failed!
[W/at.esp8266] Get dns server failed! Please check and update your firmware to support the "AT+CIPDNS_CUR?" command.
msh />tc_mqtt_basic_example start
INF|22|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(315): mqtt_sample start
INF|22|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\srmsh />
msh />c\device\src\device.c|iot_device_info_set(65): SDK_Ver: 3.0.0, Product_ID: 03UKNYBUZG, Device_Name: general_sdev1
DBG|22|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(229):  Connecting to /03UKNYBUZG.iotcloud.tencentdevices.com/8883...
DBG|22|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(234):  Setting up the SSL/TLS structure...
DBG|22|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(286): Performing the SSL/TLS handshake...
INF|23|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client.c|IOT_MQTT_Construct(114): mqtt connect with id: dSLNC success
INF|23|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(343): Cloud Device Construct Success
DBG|23|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=03UKNYBUZG/general_sdev1/data|packet_id=22073|pUserdata=h+
DBG|23|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(375): Subscribe Topic success
DBG|23|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(384): Start mqtt Loop
INF|23|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|event_handler(76): subscribe success, packet-id=22073
DBG|26|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=22074|topicName=03UKNYBUZG/general_sdev1/data|payload={"action": "publish_test", "count": "0"}
INF|27|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|event_handler(103): publish success, packet-id=22074
INF|27|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|on_message_callback(132): Receive Message With topicName:03UKNYBUZG/general_sdev1/data, payload:{"action": "publish_test", "count": "0"}
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=22075|topicName=03UKNYBUZG/general_sdev1/data|payload={"action": "publish_test", "count": "1"}
INF|31|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|event_handler(103): publish success, packet-id=22075
INF|31|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|on_message_callback(132): Receive Message With topicName:03UKNYBUZG/general_sdev1/data, payload:{"action": "publish_test", "count": "1"}
```

#### 2.4.2  MQTT 无TLS示例：
- 示例说明：该示例展示了设备和[物联网通信平台](https://cloud.tencent.com/product/iothub)的MQTT通信示例，包括MQTT连接、主题订阅、消息推送、消息收取回调处理，无TLS。对于资源特别受限的设备可能无法支持TLS，通过关闭TLS节省资源，但此中情况会面临较大的安全性问题，用户根据业务场景审慎使用。

- 配置选项
```
             --- tencent-iothub:  Tencent Cloud SDK for IoT platform          
                     Select Tencent IOT platform (Iot_hub platform)  --->     
               (03UKNYBUZG) Config Product Id                                 
               (general_sdev1) Config Device Name                             
               (VI04Eh4N8VgM29U/dnu9cQ==) Config Device Secret                
               [ ]   Enable dynamic register                                  
               [ ]   Enable err log upload                                    
               [ ]   Enable TLS/DTLS                                          
               [*]   Enable MQTT                                              
                       Enable shadow (ONLY MQTT)  --->                        
               [ ]   Enable COAP                                              
               [ ]   Enable SCENARIOS                                         
                     Version (latest)  --->
```

- 运行示例

```
 \ | /
- RT -     Thread Operating System
 / | \     4.0.2 build May 27 2019
 2006 - 2019 Copyright by rt-thread team
[I/sal.skt] Socket Abstraction Layer initialize success.
[I/at.clnt] AT client(V1.2.0) on device uart4 initialize success.
[I/at.esp8266] ESP8266 WIFI is disconnect.
[I/at.esp8266] ESP8266 WIFI is connected.
[I/at.esp8266] AT network initialize success!
msh />[E/at.clnt] execute command (AT+CIPDNS_CUR?) failed!
[W/at.esp8266] Get dns server failed! Please check and update your firmware to support the "AT+CIPDNS_CUR?" command.
msh />tc_mqtt_basic_example start
INF|18|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(315): mqtt_sample start
INF|18|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\demsh />
msh />vice\src\device.c|iot_device_info_set(65): SDK_Ver: 3.0.0, Product_ID: 03UKNYBUZG, Device_Name: general_sdev1
DBG|18|packages\tencent-iot-sdk-latest\ports\rtthread\HAL_TCP_rtthread.c|HAL_TCP_Connect(68): establish tcp connection with server(host=03UKNYBUZG.iotcloud.tencentdevices.com port=1883)
DBG|18|packages\tencent-iot-sdk-latest\ports\rtthread\HAL_TCP_rtthread.c|HAL_TCP_Connect(101): success to establish tcp, fd=4
INF|19|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client.c|IOT_MQTT_Construct(114): mqtt connect with id: ech3n success
INF|19|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(343): Cloud Device Construct Success
DBG|19|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=03UKNYBUZG/general_sdev1/data|packet_id=14966|pUserdata=P$
DBG|19|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(375): Subscribe Topic success
DBG|19|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|mqtt_basic_thread(384): Start mqtt Loop
INF|19|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|event_handler(76): subscribe success, packet-id=14966
DBG|22|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=14967|topicName=03UKNYBUZG/general_sdev1/data|payload={"action": "publish_test", "count": "0"}
INF|23|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|event_handler(103): publish success, packet-id=14967
INF|23|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|on_message_callback(132): Receive Message With topicName:03UKNYBUZG/general_sdev1/data, payload:{"action": "publish_test", "count": "0"}
DBG|26|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(329): publish topic seq=14968|topicName=03UKNYBUZG/general_sdev1/data|payload={"action": "publish_test", "count": "1"}
INF|27|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|event_handler(103): publish success, packet-id=14968
INF|27|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\mqtt_sample.c|on_message_callback(132): Receive Message With topicName:03UKNYBUZG/general_sdev1/data, payload:{"action": "publish_test", "count": "1"}
```

#### 2.4.3  Shadow + TLS示例：
- 示例说明：该示例展示了设备和[物联网通信平台](https://cloud.tencent.com/product/iothub)的基于[影子协议](https://cloud.tencent.com/document/product/634/11918)的MQTT通信示例，关于影子协议参见链接说明，使能TLS。

- 配置选项
```
             --- tencent-iothub:  Tencent Cloud SDK for IoT platform          
                     Select Tencent IOT platform (Iot_hub platform)  --->     
               (03UKNYBUZG) Config Product Id                                 
               (general_sdev1) Config Device Name                             
               (VI04Eh4N8VgM29U/dnu9cQ==) Config Device Secret                
               [ ]   Enable dynamic register                                  
               [ ]   Enable err log upload                                    
               [*]   Enable TLS/DTLS                                          
               [*]   Enable MQTT                                              
                      Enable shadow (USE SHADOW)  --->                       
               [ ]   Enable COAP                                              
               [ ]   Enable SCENARIOS                                         
                     Version (latest)  --->
```

- 运行示例

```
\ | /
- RT -     Thread Operating System
 / | \     4.0.2 build May 28 2019
 2006 - 2019 Copyright by rt-thread team
[I/sal.skt] Socket Abstraction Layer initialize success.
[I/at.clnt] AT client(V1.2.0) on device uart4 initialize success.
[I/at.esp8266] ESP8266 WIFI is disconnect.
[I/at.esp8266] ESP8266 WIFI is connected.
[I/at.esp8266] AT network initialize success!
msh />[E/at.clnt] execute command (AT+CIPDNS_CUR?) failed!
[W/at.esp8266] Get dns server failed! Please check and update your firmware to support the "AT+CIPDNS_CUR?" command.
msh />tc_shadow_example start
INF|28|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\device\src\device.c|iot_device_info_set(65): SDK_Ver: 3.0.0, Product_ID: 0msh />
msh />3UKNYBUZG, Device_Name: general_sdev1
DBG|28|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(229):  Connecting to /03UKNYBUZG.iotcloud.tencentdevices.com/8883...
DBG|28|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(234):  Setting up the SSL/TLS structure...
DBG|28|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(286): Performing the SSL/TLS handshake...
INF|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client.c|IOT_MQTT_Construct(114): mqtt connect with id: ze9u8 success
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=$shadow/operation/result/03UKNYBUZG/general_sdev1|packet_id=65500|pUserdata=(*
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_shadow_event_handler(73): shadow subscribe nack, packet-id=65500
ERR|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_common.c|_handle_suback_packet(1001): MQTT SUBSCRIBE failed, packet_id: 65500 topic: $shadow/operation/result/03UKNYBUZG/general_sdev1
ERR|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Construct(175): Sync device data failed
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=$shadow/operation/result/03UKNYBUZG/general_sdev1|packet_id=65501|pUserdata=(*
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Get(384): GET Request Document: {"clientToken":"03UKNYBUZG-0"}
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$shadow/operation/03UKNYBUZG/general_sdev1|payload={"type":"get", "clientToken":"03UKNYBUZG-0"}
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_shadow_event_handler(63): shadow subscribe success, packet-id=65501
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_update_ack_cb(114): requestAck=0
DBG|31|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_update_ack_cb(117): Received Json Document={"clientToken":"03UKNYBUZG-0","payload":{"metadata":{"delta":{"testdata":{"timestamp":1552468381105},"tt":{"timestamp":1552468381105}},"desired":{"testdata":{"timestamp":1552468381105},"tt":{"timestamp":1552468381105}},"reported":{"energyConsumption":{"timestamp":1558444250381},"updateCount":{"timestamp":1558959785957}}},"state":{"delta":{"testdata":"hello_world","tt":1},"desired":{"testdata":"hello_world","tt":1},"reported":{"energyConsumption":0,"updateCount":5546}},"timestamp":1558959785957,"version":18390},"result":0,"timestamp":1559012560,"type":"get"}

DBG|35|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=$shadow/operation/result/03UKNYBUZG/general_sdev1|packet_id=65502|pUserdata=(*
DBG|35|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Update(318): UPDATE Request Document: {"version":18390, "state":{"reported":{"updateCount":0}}, "clientToken":"03UKNYBUZG-1"}
DBG|35|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$shadow/operation/03UKNYBUZG/general_sdev1|payload={"type":"update", "version":18390, "state":{"reported":{"updateCount":0}}, "clientToken":"03UKNYBUZG-1"}
WRN|36|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_common.c|_handle_suback_packet(1014): There is a identical topic and related handle in list!
DBG|36|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_shadow_event_handler(63): shadow subscribe success, packet-id=65502
INF|36|packages\tencent-iot-sdk-latest\samples\iot_hub_platform\shadow_sample.c|OnShadowUpdateCallback(58): recv shadow update response, response ack: 0
```

#### 2.4.4 数据模板智能灯 + TLS示例：
- 示例说明：该示例展示了设备和[物联网开发平台](https://cloud.tencent.com/product/iotexplorer)的基于[数据模板协议](https://cloud.tencent.com/document/product/1081/34916)通信示例，关于数据模板协议参见链接说明，使能TLS，示例在物联网开发平台下发控制灯为红色的命令，设备端收取了消息，打印颜色，并上报对应消息。

- 配置选项
```
      --- tencent-iothub:  Tencent Cloud SDK for IoT platform                               
                 Select Tencent IOT platform (Iot_explorer platform)  --->                       
           (4BZCJGQ1U1) Config Product Id                                                        
           (dev1) Config Device Name                                                             
           (8cY3SFxExhjYTfvuc6kn7g==) Config Device Secret                                       
           [ ]   Enable dynamic register                                                         
           [ ]   Enable err log upload                                                           
           [*]   Enable TLS/DTLS                                                                 
           -*-   Enable data template                                                            
                   Enable event (USE event)  --->                                                
           [*]   Enable SCENARIOS                                                                
                   Select SCENARIOS (Use Scenario light data template)  --->                     
                 Version (latest)  --->
```

- 运行示例

```
\ | /
- RT -     Thread Operating System
 / | \     4.0.2 build May 28 2019
 2006 - 2019 Copyright by rt-thread team
[I/sal.skt] Socket Abstraction Layer initialize success.
[I/at.clnt] AT client(V1.2.0) on device uart4 initialize success.
[I/at.esp8266] ESP8266 WIFI is disconnect.
[I/at.esp8266] ESP8266 WIFI is connected.
[I/at.esp8266] AT network initialize success!
msh />[E/at.clnt] execute command (AT+CIPDNS_CUR?) failed!
[W/at.esp8266] Get dns server failed! Please check and update your firmware to support the "AT+CIPDNS_CUR?" command.
msh />tc_data_template_light_example start
INF|28|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\device\src\device.c|iot_device_info_set(65): SDK_Ver: 3.0.0, Product_ID: 4BZCJGQ1U1, Device_Name: dev1
msh />
msh />DBG|28|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(229):  Connecting to /4BZCJGQ1U1.iotcloud.tencentdevices.com/8883...
DBG|29|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(234):  Setting up the SSL/TLS structure...
DBG|29|packages\tencent-iot-sdk-latest\ports\ssl\HAL_TLS_mbedtls.c|HAL_TLS_Connect(286): Performing the SSL/TLS handshake...
INF|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client.c|IOT_MQTT_Construct(114): mqtt connect with id: ze9u8 success
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=$template/operation/result/4BZCJGQ1U1/dev1|packet_id=65500|pUserdata=0@
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_shadow_event_handler(63): shadow subscribe success, packet-id=65500
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|event_handler(215): subscribe success, packet-id=65500
INF|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Construct(173): Sync device data successfully
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(494): Cloud Device Construct Success
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_subscribe.c|qcloud_iot_mqtt_subscribe(139): topicName=$thing/down/event/4BZCJGQ1U1/dev1|packet_id=65501|pUserdata=0@
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|_register_data_template_property(371): data template property=power_switch registered.
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|_register_data_template_property(371): data template property=color registered.
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|_register_data_template_property(371): data template property=brightness registered.
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|_register_data_template_property(371): data template property=name registered.
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(515): Register data template propertys Success
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Get(384): GET Request Document: {"clientToken":"4BZCJGQ1U1-0"}
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$template/operation/4BZCJGQ1U1/dev1|payload={"type":"get", "clientToken":"4BZCJGQ1U1-0"}
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_shadow_event_handler(63): shadow subscribe success, packet-id=65501
INF|30|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|event_handler(215): subscribe success, packet-id=65501
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_update_ack_cb(114): requestAck=0
DBG|30|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_update_ack_cb(117): Received Json Document={"clientToken":"4BZCJGQ1U1-0","payload":{"metadata":{"reported":{"brightness":{"timestamp":1558007562228},"color":{"timestamp":1558007562228},"name":{"timestamp":1558007562228},"power_switch":{"timestamp":1558007562228}}},"state":{"reported":{"brightness":29,"color":1,"name":"dev1","power_switch":1}},"timestamp":1558007562228,"version":41},"result":0,"timestamp":1559014360,"type":"get"}

DBG|33|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(600): cycle report:{"version":41, "state":{"reported":{"power_switch":0,"color":0,"brightness":0.000000,"name":"dev1"}}, "clientToken":"4BZCJGQ1U1-1"}
DBG|33|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Update(318): UPDATE Request Document: {"version":41, "state":{"reported":{"power_switch":0,"color":0,"brightness":0.000000,"name":"dev1"}}, "clientToken":"4BZCJGQ1U1-1"}
DBG|33|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$template/operation/4BZCJGQ1U1/dev1|payload={"type":"update", "version":41, "state":{"reported":{"power_switch":0,"color":0,"brightness":0.000000,"name":"dev1"}}, "clientToken":"4BZCJGQ1U1-1"}
INF|33|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(605): shadow update(reported) success
INF|36|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|OnShadowUpdateCallback(353): recv shadow update response, response ack: 0
DBG|49|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client_manager.c|_on_operation_result_handler(313): dlta:{"brightness":28,"power_switch":1}
INF|49|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|OnDeltaTemplateCallback(342): Property=brightness changed
INF|49|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|OnDeltaTemplateCallback(342): Property=power_switch changed
[  lighting  ]|[color: RED ]|[brightness:|||||---------------]|[dev1]
DBG|49|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Update(318): UPDATE Request Document: {"version":45, "state":{"desired": null}, "clientToken":"4BZCJGQ1U1-4"}
DBG|49|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$template/operation/4BZCJGQ1U1/dev1|payload={"type":"update", "version":45, "state":{"desired": null}, "clientToken":"4BZCJGQ1U1-4"}
DBG|50|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_update_ack_cb(114): requestAck=0
DBG|50|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|_update_ack_cb(117): Received Json Document={"clientToken":"4BZCJGQ1U1-4","payload":{"metadata":null,"state":null,"timestamp":1559014380063,"version":46},"result":0,"timestamp":1559014380063,"type":"update"}

INF|50|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(556): shadow update(desired) success
DBG|50|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(574): report:{"type":"update", "version":45, "state":{"desired": null}, "clientToken":"4BZCJGQ1U1-4"}
DBG|50|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Update(318): UPDATE Request Document: {"version":46, "state":{"reported":{"power_switch":1,"brightness":28.000000}}, "clientToken":"4BZCJGQ1U1-5"}
DBG|50|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$template/operation/4BZCJGQ1U1/dev1|payload={"type":"update", "version":46, "state":{"reported":{"power_switch":1,"brightness":28.000000}}, "clientToken":"4BZCJGQ1U1-5"}
INF|50|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(581): shadow update(reported) success
DBG|50|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$thing/up/event/4BZCJGQ1U1/dev1|payload={"method":"event_post", "clientToken":"4BZCJGQ1U1-6", "eventId":"status_report", "type":"info", "timestamp":1514764850000, "params":{"status":1,"message":"light off"}}
INF|53|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|OnShadowUpdateCallback(353): recv shadow update response, response ack: 0
INF|53|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\event\src\qcloud_iot_event.c|_on_event_reply_callback(96): Receive Message With topicName:$thing/down/event/4BZCJGQ1U1/dev1, payload:{"method":"event_reply","clientToken":"4BZCJGQ1U1-6","code":405,"status":"status_report too old","data":{}}
DBG|53|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\event\src\qcloud_iot_event.c|_on_event_reply_callback(115): eventToken:4BZCJGQ1U1-6 code:405 status:status_report too old
DBG|53|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|event_post_cb(189): Reply:{"method":"event_reply","clientToken":"4BZCJGQ1U1-6","code":405,"status":"status_report too old","data":{}}
DBG|53|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\event\src\qcloud_iot_event.c|_release_reply_request(78): eventToken[4BZCJGQ1U1-6] released
DBG|53|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(600): cycle report:{"version":47, "state":{"reported":{"power_switch":1,"color":0,"brightness":28.000000,"name":"dev1"}}, "clientToken":"4BZCJGQ1U1-7"}
DBG|53|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\shadow\src\shadow_client.c|IOT_Shadow_Update(318): UPDATE Request Document: {"version":47, "state":{"reported":{"power_switch":1,"color":0,"brightness":28.000000,"name":"dev1"}}, "clientToken":"4BZCJGQ1U1-7"}
DBG|53|packages\tencent-iot-sdk-latest\qcloud-iot-sdk-embedded-c\src\mqtt\src\mqtt_client_publish.c|qcloud_iot_mqtt_publish(337): publish packetID=0|topicName=$template/operation/4BZCJGQ1U1/dev1|payload={"type":"update", "version":47, "state":{"reported":{"power_switch":1,"color":0,"brightness":28.000000,"name":"dev1"}}, "clientToken":"4BZCJGQ1U1-7"}
INF|54|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|data_template_light_thread(605): shadow update(reported) success
INF|57|packages\tencent-iot-sdk-latest\samples\iot_explorer_platform\light_data_template_sample.c|OnShadowUpdateCallback(353): recv shadow update response, response ack: 0
```

### 2.5 服务端下行消息控制
- [物联网开发平台](https://cloud.tencent.com/product/iotexplorer)可以通过控制台直接调试，如下截图
![control](https://main.qcloudimg.com/raw/137d8f6df2d6d5df21a2507412392360.jpg)

- [物联网通信平台](https://cloud.tencent.com/product/iothub)通过控制台可以下发基于影子的消息，但操作相对复杂。建议通过在线API调试，参考文档[服务下发消息到设备](https://cloud.tencent.com/document/product/634/34806)


### 2.6 其他示例说明
 关于 SDK 的更多使用方式及接口了解, 参见 `qcloud_iot_api_export.h`，其他示例不再一一列举，开发者也可以把官网SDK下的其他Sample参照port目录移植过来，譬如OTA、网关、动态注册等等。

### 2.7 可变参数配置
开发者可以根据具体场景需求，配置相应的参数，满足实际业务的运行。可变接入参数包括：
1. MQTT 心跳消息发送周期, 单位: ms 
2. MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间, 单位:ms。 建议 5000 ms
3. TLS 连接握手超时时间, 单位: ms
4. MQTT 协议发送消息和接受消息的 buffer 大小默认是 512 字节，最大支持 256 KB
5. CoAP 协议发送消息和接受消息的 buffer 大小默认是 512 字节，最大支持 64 KB
6. 重连最大等待时间

修改 qcloud_iot_export.h 文件如下宏定义可以改变对应接入参数的配置。

```
/* MQTT心跳消息发送周期, 单位:ms */
#define QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL                         (240 * 1000)

/* MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间, 单位:ms 建议5000ms */
#define QCLOUD_IOT_MQTT_COMMAND_TIMEOUT                             (5000)

/* TLS连接握手超时时间, 单位:ms */
#define QCLOUD_IOT_TLS_HANDSHAKE_TIMEOUT                            (5000)

/* MQTT消息发送buffer大小, 支持最大256*1024 */
#define QCLOUD_IOT_MQTT_TX_BUF_LEN                                  (512)

/* MQTT消息接收buffer大小, 支持最大256*1024 */
#define QCLOUD_IOT_MQTT_RX_BUF_LEN                                  (512)

/* COAP 发送消息buffer大小，最大支持64*1024字节 */
#define COAP_SENDMSG_MAX_BUFLEN                                     (512)

/* COAP 接收消息buffer大小，最大支持64*1024字节 */
#define COAP_RECVMSG_MAX_BUFLEN                                     (512)

/* 重连最大等待时间 */
#define MAX_RECONNECT_WAIT_INTERVAL                                 (60000)

```