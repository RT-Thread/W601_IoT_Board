/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-05-29     ZeroFree     first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>

#include "drv_wifi.h"
#include "wifi_config.h"
#include <easyflash.h>
#include <fal.h>

#include <librws.h>
#include <string.h>
#include <stdlib.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define RWS_RECEIVE_TIMEOUT (15 * 1000)

static struct rt_semaphore rws_sem_rec;
static char *text_rec;

static rws_socket rws_connect(const char *host, const int port);
static void rws_disconnect(rws_socket socket);
static int rws_send(rws_socket socket, const char *text);

int main(void)
{
    int i;
    static rws_socket socket;
    /* ！！！注意：要换成自己电脑的 IP 地址 */
    const char *host = "192.168.10.168";
    const char *send_text = "Hello RT-Thread!";

    /* 初始化分区表 */
    fal_init();
    /* 初始化 easyflash */
    easyflash_init();
    /* 配置 wifi 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    /* 初始化自动连接配置 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 查询 wlan 是否处于连接状态 */
    while (rt_wlan_is_ready() != RT_TRUE)
    {
        rt_thread_delay(1000);
    }
    rt_sem_init(&rws_sem_rec, "rws_rec", 0, RT_IPC_FLAG_FIFO);
    /* 连接 echo.websocket.org */
    socket = rws_connect(host, 9999);
    if (RT_NULL == socket)
    {
        LOG_E("Can not connect %s",host);
        return 0;
    }

    for (i = 0; i < 10; i++)
    {
        /* 发送消息 */
        rws_send(socket, send_text);
        /* 等待接收完成 */
        rt_sem_take(&rws_sem_rec, RWS_RECEIVE_TIMEOUT);
        /* 比较发送消息与接收消息是否一致 */
        if (strcmp(text_rec, send_text) != 0)
        {
            LOG_E("Receive data: %s is different from send data: %s", text_rec, send_text);
        }
    }

    if (text_rec != RT_NULL)
    {
        rt_free(text_rec);
    }
    /* 断开连接 */
    rws_disconnect(socket);
}

static void rws_open(rws_socket socket)
{
    LOG_D("Websocket open success.");
}

static void rws_close(rws_socket socket)
{
    rws_error error = rws_socket_get_error(socket);

    if (error)
    {
        LOG_E("Websocket disconnect, error: %i, %s ", rws_error_get_code(error), rws_error_get_description(error));
    }
    else
    {
        LOG_D("Websocket disconnect! ");
    }

    rws_socket_disconnect_and_release(socket);
}

static void message_text_rec(rws_socket socket, const char *text, const unsigned int len)
{
    if (text_rec != RT_NULL)
    {
        rt_free(text_rec);
    }
    text_rec = (char *)rt_malloc(2048);

    rt_memset(text_rec, 0x00, 2048);
    rt_memcpy(text_rec, text, len);

    LOG_D("Receive message: %s, length: %d ", text_rec, len);
    /* 通知接收完成 */
    rt_sem_release(&rws_sem_rec);
}

static rws_socket rws_connect(const char *host, const int port)
{
    static rws_socket socket;

    /* 创建 socket */
    socket = rws_socket_create();
    if (socket == RT_NULL)
    {
        LOG_E("Librws socket create failed. ");
        return RT_NULL;
    }
    /* 设置 socket url */
    rws_socket_set_url(socket, "ws", host, port, "/");
    /* 设置 socket 连接回调函数 */
    rws_socket_set_on_connected(socket, &rws_open);
    /* 设置socket 断开连接回调函数 */
    rws_socket_set_on_disconnected(socket, &rws_close);
    /* 设置接收消息回调函数 */
    rws_socket_set_on_received_text(socket, &message_text_rec);
    /* 设置自定义模式 */
    rws_socket_set_custom_mode(socket);

    if (rws_socket_connect(socket) == RT_FALSE)
    {
        LOG_E("Connect ws://%s:%d/ failed. ", host, port);
        return RT_NULL;
    }
    LOG_D("Connect ws://%s:%d/ success.", host, port);

    return socket;
}

static void rws_disconnect(rws_socket socket)
{
    rws_socket_disconnect_and_release(socket);
    LOG_D("Try disconnect websocket connection.");
}

static int rws_send(rws_socket socket, const char *text)
{
    LOG_D("send message: %s, len: %d", text, rt_strlen(text));
    rws_socket_send_text(socket, text);

    return RT_EOK;
}
