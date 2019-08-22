/*
 *   Copyright (c) 2014 - 2017 Kulykov Oleh <info@resident.name>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "librws.h"
#include "rws_socket.h"
#include "rws_memory.h"
#include "rws_string.h"
#include <sys/time.h>
#include <netdb.h>

/* Todo: 需要导出到Kconfig中的 */
#define RWS_CONNECT_RETRY_DELAY ( 200) /* 重试延时(单位?) */
#define RWS_CONNECT_ATTEMPS     (   5) /* 尝试次数? */
#define RWS_RECEIVE_BUFF_SIZE   (2048) /* 接受buff分配内存大小 */

#ifndef RWS_OS_WINDOWS
#define WSAEWOULDBLOCK EAGAIN
#define WSAEINPROGRESS EINPROGRESS
#endif

#if ((DBG_LEVEL) == (DBG_LOG))
    static void hexdump(const rt_uint8_t *p, rt_size_t len);
#endif

unsigned int rws_socket_get_next_message_id(_rws_socket *s)
{
    const unsigned int mess_id = ++s->next_message_id;

    if (mess_id > 9999999)
    {
        s->next_message_id = 0;
    }

    return mess_id;
}

void rws_socket_send_ping(_rws_socket *s)
{
    char buff[16];
    size_t len = 0;
    _rws_frame *frame = rws_frame_create();

    len = rws_sprintf(buff, 16, "%u", rws_socket_get_next_message_id(s));

    frame->is_masked = rws_true;
    frame->opcode = rws_opcode_ping;
    rws_frame_fill_with_send_data(frame, buff, len);
    rws_socket_append_send_frames(s, frame);
}

void rws_socket_inform_recvd_frames(_rws_socket *s)
{
    rws_bool is_all_finished = rws_true;
    _rws_frame *frame = NULL;
    _rws_node *cur = s->recvd_frames;

    while (cur)
    {
        frame = (_rws_frame *)cur->value.object;
        if (frame)
        {
            if (frame->is_finished)
            {
                switch (frame->opcode)
                {
                case rws_opcode_text_frame:
                    if (s->on_recvd_text)
                    {
                        s->on_recvd_text(s, (const char *)frame->data, (unsigned int)frame->data_size);
                    }
                    break;
                case rws_opcode_binary_frame:
                    if (s->on_recvd_bin)
                    {
                        s->on_recvd_bin(s, frame->data, (unsigned int)frame->data_size);
                    }
                    break;
                default:
                    break;
                }

                rws_frame_delete(frame);
                cur->value.object = NULL;
            }
            else
            {
                is_all_finished = rws_false;
            }
        }
        cur = cur->next;
    }
    if (is_all_finished)
    {
        rws_list_delete_clean(&s->recvd_frames);
    }
}

void rws_socket_read_handshake_responce_value(const char *str, char **value)
{
    const char *s = NULL;
    size_t len = 0;

    while (*str == ':' || *str == ' ')
    {
        str++;
    }
    s = str;
    while (*s != '\r' && *s != '\n')
    {
        s++;
        len++;
    }
    if (len > 0)
    {
        *value = rws_string_copy_len(str, len);
    }
}

rws_bool rws_socket_process_handshake_responce(_rws_socket *s)
{
    const char *str = (const char *)s->received;
    const char *sub = NULL;
    float http_ver = -1;
    int http_code = -1;

    rws_error_delete_clean(&s->error);
    sub = strstr(str, "HTTP/");
    if (!sub)
    {
        return rws_false;
    }

    sub += 5;
    if (rws_sscanf(sub, "%f %i", &http_ver, &http_code) != 2)
    {
        http_ver = -1;
        http_code = -1;
    }

    // "Sec-WebSocket-Accept"
    sub = strstr(str, k_rws_socket_sec_websocket_accept);
    if (sub)
    {
        sub += strlen(k_rws_socket_sec_websocket_accept);
        rws_socket_read_handshake_responce_value(sub, &s->sec_ws_accept);
    }

    // "Sec-WebSocket-Accept"
    sub = strstr(str, "sec-websocket-accept");
    if (sub)
    {
        sub += strlen(k_rws_socket_sec_websocket_accept);
        rws_socket_read_handshake_responce_value(sub, &s->sec_ws_accept);
    }

    if (http_code != 101 || !s->sec_ws_accept)
    {
        s->error = rws_error_new_code_descr(rws_error_code_parse_handshake,
                                            (http_code != 101) ? "HTPP code not found or non 101" : "Accept key not found");
        return rws_false;
    }
    return rws_true;
}

// need close socket on error
rws_bool rws_socket_send(_rws_socket *s, const void *data, const size_t data_size)
{
    int sended = -1, error_number = -1;
    rws_error_delete_clean(&s->error);

#ifdef LIBRWS_USING_MBED_TLS
    if (s->scheme && strcmp(s->scheme, "wss") == 0)
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] try send data with tls", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        sended = mbedtls_ssl_write(&(s->ssl->ssl_ctx), (const unsigned char *)data, (int)data_size);
    }
    else
#endif /* LIBRWS_USING_MBED_TLS */
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] try send data", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        sended = (int)send(s->socket, data, (int)data_size, 0);
    }

    if (sended > 0)
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] send %d bytes data", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, sended);
    }
    else
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] send error = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, sended);
    }

#if defined(RWS_OS_WINDOWS)
    error_number = WSAGetLastError();
#else
    error_number = errno;
#endif
    LOG_D("[T:%.8d, L:%.5d, %*.*s] current error_number = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, error_number);

    if (sended > 0)
    {
        return rws_true;
    }

    rws_socket_check_write_error(s, error_number);
    if (s->error)
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] send error close websocket connection, %s", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->error->description);
        rws_socket_close(s);
        return rws_false;
    }
    return rws_true;
}

rws_bool rws_socket_recv(_rws_socket *s)
{
    rws_bool result = rws_true;
    int is_reading = 1, error_number = -1, len = -1;
    char *received = NULL;
    size_t total_len = 0;
    // char *buff = 0;

    LOG_D("[T:%.8d, L:%.5d, %*.*s] receive data", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    rws_error_delete_clean(&s->error);
    LOG_D("[T:%.8d, L:%.5d, %*.*s] delete error info", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

    // buff = rws_malloc(RWS_RECEIVE_BUFF_SIZE);
    if (!s->payload_buff)
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] recv buff malloc fail, no memory", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        result = rws_false;
        goto _exit;
    }
    else
    {
        rt_memset(s->payload_buff, 0x00, RWS_RECEIVE_BUFF_SIZE);
    }

    while (is_reading)
    {
#ifdef LIBRWS_USING_MBED_TLS
        if (s->scheme && strcmp(s->scheme, "wss") == 0)
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] try recv data with tls", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            len = mbedtls_ssl_read(&(s->ssl->ssl_ctx), (unsigned char *)(s->payload_buff), RWS_RECEIVE_BUFF_SIZE);
        }
        else
#endif /* LIBRWS_USING_MBED_TLS */
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] try recv data", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            len = (int)recv(s->socket, s->payload_buff, RWS_RECEIVE_BUFF_SIZE, 0);
        }

        if (len >= 0)
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] recv %d bytes data", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len);
        }
        else if (len == -1)
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] no data recv", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        }

#ifdef LIBRWS_USING_MBED_TLS
        else if ((len != MBEDTLS_ERR_SSL_WANT_READ) && (len != MBEDTLS_ERR_NET_RECV_FAILED))
        {
            LOG_E("[T:%.8d, L:%.5d, %*.*s] recv error = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len);
        }
#else
        else
        {
            LOG_E("[T:%.8d, L:%.5d, %*.*s] recv error = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len);
        }
#endif /* LIBRWS_USING_MBED_TLS */

#if defined(RWS_OS_WINDOWS)
        error_number = WSAGetLastError();
#else
        error_number = errno;
#endif
        LOG_D("[T:%.8d, L:%.5d, %*.*s] current error_number = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, error_number);

        if (len > 0)
        {
            total_len += len;
            if (s->received_size - s->received_len < len)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] s->received_size = %d, s->received_len = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->received_size, s->received_len);
                LOG_D("[T:%.8d, L:%.5d, %*.*s] reallocate (s->received) memory, size = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->received_size + len);
                rws_socket_resize_received(s, s->received_size + len);
            }
            received = (char *)s->received;
            if (s->received_len)
            {
                received += s->received_len;
            }
            memcpy(received, s->payload_buff, len);
            s->received_len += len;
        }
        else
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] ready exit recv complete", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            is_reading = 0;
        }
    }

    if (error_number != WSAEWOULDBLOCK && error_number != WSAEINPROGRESS && error_number != 0)
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] recv data failed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

        s->error = rws_error_new_code_descr(rws_error_code_read_write_socket, "Failed read/write socket");
        // rws_socket_close(s);

        result = rws_false;
    }
    else
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] recv data complete", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    }

_exit:
    return result;
}

/* RT-Thread Team add */
#if ((DBG_LEVEL) == (DBG_LOG))
static void hexdump(const rt_uint8_t *p, rt_size_t len)
{
    unsigned char *buf = (unsigned char *)p;
    int i, j;

    rt_kprintf("Dump 0x%.8x %dBytes\n", (int)p, len);
    rt_kprintf("Offset    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (i = 0; i < len; i += 16)
    {
        rt_kprintf("%08X: ", i + (int)p);

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                rt_kprintf("%02X ", buf[i + j]);
            }
            else
            {
                rt_kprintf("   ");
            }
        }
        rt_kprintf(" ");

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                rt_kprintf("%c", ((unsigned int)((buf[i + j]) - ' ') < 127u - ' ') ? buf[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
}
#endif

rws_bool rws_socket_recv_custom(_rws_socket *s)
{
    rws_bool result = rws_true;
    int error_number = -1, len = -1;

    //size_t frame_len   = 0;                       /* 需要接收的数据帧长度 */

    // char *header_buff  = 0;                      /* 帧头buff */
    size_t header_len  = 0;                         /* 帧头长度 */
    size_t header_ptr  = 0;                         /* 帧头buff写指针 */

    // char *payload_buff = 0;                      /* 负载buff */
    size_t payload_len = 0;                         /* 负载长度 */
    size_t payload_ptr = 0;                         /* 负载buff写指针 */

    size_t total_len   = 0;                         /* 当前接收的总长度 */
    rws_opcode opcode  = rws_opcode_continuation;   /* 解析出来的数据帧类型 */
    //int is_parsed      = 0;                       /* 帧头是否解析完毕 */

    LOG_D("[T:%.8d, L:%.5d, %*.*s] receive data", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    rws_error_delete_clean(&s->error);
    LOG_D("[T:%.8d, L:%.5d, %*.*s] delete error info", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

    /* 分配帧头buff */
    // header_buff = rws_malloc(RWS_RECEIVE_HEADER_BUFF_SIZE);
    if (!s->header_buff)
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] header buff is null", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        result = rws_false;
        goto _exit;
    }
    else
    {
        rt_memset(s->header_buff, 0x00, RWS_RECEIVE_HEADER_BUFF_SIZE);
    }

    /* 分配负载buff */
    // payload_buff = rws_malloc(RWS_RECEIVE_PAYLOAD_BUFF_SIZE);
    if (!s->payload_buff)
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] payload buff is null", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        result = rws_false;
        goto _exit;
    }
    else
    {
        rt_memset(s->payload_buff, 0x00, RWS_RECEIVE_PAYLOAD_BUFF_SIZE);
    }

    /* 尝试从TCP中读取数据帧头进行解析 */
    size_t overlength = 2;

    while (overlength > 0)
    {
        /* 这里读取2字节的帧头 */
#ifdef LIBRWS_USING_MBED_TLS
        if (s->scheme && strcmp(s->scheme, "wss") == 0)
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] try read header 2 bytes data in tls mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            len = mbedtls_ssl_read(&(s->ssl->ssl_ctx), (unsigned char *)s->header_buff + header_ptr, overlength);
        }
        else
#endif /* LIBRWS_USING_MBED_TLS */
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] try read header 2 bytes data in normal mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            len = (int)recv(s->socket, s->header_buff + header_ptr, overlength, 0);
        }

        if (len > 0)
        {
            overlength -= len;
            total_len += len;
            header_ptr += len;
            LOG_D("[T:%.8d, L:%.5d, %*.*s] read %d byte(s) date, overlength is %d, total read len is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len, overlength, total_len);
        }

        /* 第一次就没有读到数据就退出接收, 等待下一次啊 IDLE 进入后再尝试读取数据 */
        else
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] no data, quit recv, wait until next IDLE to try", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            goto _quit;
        }
    }

    LOG_D("[T:%.8d, L:%.5d, %*.*s] read header 2 bytes date succeed, [%.2x, %.2x]", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->header_buff[0], s->header_buff[1]);

    /* 尝试解析帧头数据有多少字节 */
    header_len = rws_frame_get_header_size(s->header_buff, 2);
    LOG_D("[T:%.8d, L:%.5d, %*.*s] header len is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, header_len);

    /* 尝试解析数据的格式 */
    opcode = rws_frame_get_opcode(s->header_buff, 2);

    switch (opcode) /* 打印数据帧格式 */
    {
    case rws_opcode_continuation:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = 0(rws_opcode_continuation)", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        break;

    case rws_opcode_text_frame:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = 1(rws_opcode_text_frame)", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        break;

    case rws_opcode_binary_frame:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = 2(rws_opcode_binary_frame)", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        break;

    case rws_opcode_connection_close:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = 8(rws_opcode_connection_close)", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        break;

    case rws_opcode_ping:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = 9(rws_opcode_ping)", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        break;

    case rws_opcode_pong:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = 10(rws_opcode_pong)", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        break;

    default:
        LOG_D("[T:%.8d, L:%.5d, %*.*s] opcode = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, opcode);
        break;
    }

    /* 尝试解析负载数据有多少字节 */
    const unsigned int payload = (s->header_buff[1] & 0x7f);

    /* 说明帧头只有2字节, s->header_buff[1] 中第7位就是 payload 负载数据长度 */
    if (payload < 126)
    {
        payload_len = payload;
    }

    /* 说明帧头有扩张2字节, s->header_buff[2..3] 中就是 payload 负载数据长度 */
    else if (payload == 126)
    {
        size_t payload_expand_overlength = 2;

        /* 帧长有2字节的扩展, 阻塞式再读2字节扩展 */
        while (payload_expand_overlength > 0)
        {
#ifdef LIBRWS_USING_MBED_TLS
            if (s->scheme && strcmp(s->scheme, "wss") == 0)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] try read header payload length data in tls mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                len = mbedtls_ssl_read(&(s->ssl->ssl_ctx), (unsigned char *)(s->header_buff) + header_ptr, payload_expand_overlength);
            }
            else
#endif /* LIBRWS_USING_MBED_TLS */
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] try read header payload length data in normal mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                len = (int)recv(s->socket, s->header_buff + header_ptr, payload_expand_overlength, 0);
            }

            if (len > 0)
            {
                payload_expand_overlength -= len;
                total_len += len;
                header_ptr += len;
                LOG_D("[T:%.8d, L:%.5d, %*.*s] read %d byte(s) date, payload_expand_overlength is %d, total read len is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len, payload_expand_overlength, total_len);
            }
            else
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] no data, then try to read", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }

        payload_len |= ((unsigned int)(s->header_buff[2])) << 8;
        payload_len |= ((unsigned int)(s->header_buff[3])) << 0;
    }

    else if (payload == 127)
    {
        /* Todo */
        LOG_D("[T:%.8d, L:%.5d, %*.*s] Todo", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        while (1);
    }

    LOG_D("[T:%.8d, L:%.5d, %*.*s] payload len is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, payload_len);

    /* 读取帧头剩余字节数据 */
    size_t frame_header_overlength = header_len - total_len;

    while (frame_header_overlength > 0)
    {
#ifdef LIBRWS_USING_MBED_TLS
        if (s->scheme && strcmp(s->scheme, "wss") == 0)
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] try read header overlength data in tls mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            len = mbedtls_ssl_read(&(s->ssl->ssl_ctx), (unsigned char *)(s->header_buff) + header_ptr, frame_header_overlength);
        }
        else
#endif /* LIBRWS_USING_MBED_TLS */
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] try read header overlength data in normal mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            len = (int)recv(s->socket, s->header_buff + header_ptr, frame_header_overlength, 0);
        }

        if (len > 0)
        {
            frame_header_overlength -= len;
            total_len += len;
            header_ptr += len;
            LOG_D("[T:%.8d, L:%.5d, %*.*s] read %d byte(s) date, frame_header_overlength is %d, total read len is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len, frame_header_overlength, total_len);
        }
        else
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] no data, then try to read", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        }
    }

    /* 正常解析负载数据 */
    total_len = 0;
    size_t payload_overlength = payload_len;
    size_t unpack_overlength = RWS_RECEIVE_PAYLOAD_BUFF_SIZE;

    /* 触发空数据帧回调 */
    if (payload_overlength == 0)
    {
        if (opcode == rws_opcode_continuation)
        {
            LOG_I("[T:%.8d, L:%.5d, %*.*s] bin frame data addr = 0x%.8x, len = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->payload_buff, payload_ptr);

            /* 调试信息 */
#if ((DBG_LEVEL) == (DBG_LOG))
            LOG_I("[T:%.8d, L:%.5d, %*.*s] bin content: ", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            hexdump(s->payload_buff, 16);
#endif

            if (s->on_recvd_bin)
            {
                LOG_I("[T:%.8d, L:%.5d, %*.*s] call bin callback", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                s->on_recvd_bin(s, s->payload_buff, (unsigned int)(payload_ptr));
                LOG_I("[T:%.8d, L:%.5d, %*.*s] call bin callback end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }
    }

    /* 非空数据帧 */
    while (payload_overlength > 0)
    {
        payload_ptr = 0;

        /* 这里是按照天猫bin音频数据2K进行拆包回调的 */
        if (payload_overlength < RWS_RECEIVE_PAYLOAD_BUFF_SIZE)
        {
            unpack_overlength = payload_overlength;
        }
        else
        {
            unpack_overlength = RWS_RECEIVE_PAYLOAD_BUFF_SIZE;
        }

        /* 阻塞式将负载数据的2K读出来 */
        while (unpack_overlength > 0)
        {
#ifdef LIBRWS_USING_MBED_TLS
            if (s->scheme && strcmp(s->scheme, "wss") == 0)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] try read payload data with tls mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                len = mbedtls_ssl_read(&(s->ssl->ssl_ctx), (unsigned char *)(s->payload_buff) + payload_ptr, unpack_overlength);
            }
            else
#endif /* LIBRWS_USING_MBED_TLS */
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] try read payload data with normal mode", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                len = (int)recv(s->socket, s->payload_buff + payload_ptr, unpack_overlength, 0);
            }

            if (len > 0)
            {
                unpack_overlength -= len;
                payload_overlength -= len;
                payload_ptr += len;
                total_len += len;
                LOG_D("[T:%.8d, L:%.5d, %*.*s] read %d byte(s) date, unpack_overlength is %d, payload_overlength is %d, total read len is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, len, unpack_overlength, payload_overlength, total_len);
            }

            /* 读不出来数据有可能是2种情况, 1: 帧结束了, 2: 数据帧未结束但是数据还没到来 */
            else
            {
                /* 读取到的数据已经大于等于帧的长度 */
                if (total_len >= payload_len)
                {
                    /* 这里最后的拆包数据需要产生回调 */
                    goto _callback;
                }
                else
                {
                    LOG_D("[T:%.8d, L:%.5d, %*.*s] no data, then try to read", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                }
            }
        }

_callback:
        /* 2K数据接收完毕被拆分, 触发回调 */
        if (opcode == rws_opcode_text_frame)
        {
            LOG_I("[T:%.8d, L:%.5d, %*.*s] txt frame data addr = 0x%.8x, len = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->payload_buff, payload_ptr);
            LOG_I("[T:%.8d, L:%.5d, %*.*s] txt content = %*.*s ", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, payload_ptr, payload_ptr, s->payload_buff);

            if (s->on_recvd_text)
            {
                LOG_I("[T:%.8d, L:%.5d, %*.*s] call txt callback", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                s->on_recvd_text(s, s->payload_buff, (unsigned int)(payload_ptr));
                LOG_I("[T:%.8d, L:%.5d, %*.*s] call txt callback end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }
        else if (opcode == rws_opcode_binary_frame || opcode == rws_opcode_continuation) /* Todo */
        {
            LOG_I("[T:%.8d, L:%.5d, %*.*s] bin frame data addr = 0x%.8x, len = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->payload_buff, payload_ptr);

            /* 调试信息 */
#if ((DBG_LEVEL) == (DBG_LOG))
            LOG_I("[T:%.8d, L:%.5d, %*.*s] bin content: ", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            hexdump(s->payload_buff, 16);
#endif

            if (s->on_recvd_bin)
            {
                LOG_I("[T:%.8d, L:%.5d, %*.*s] call bin callback", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                s->on_recvd_bin(s, s->payload_buff, (unsigned int)(payload_ptr));
                LOG_I("[T:%.8d, L:%.5d, %*.*s] call bin callback end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }
    }

_quit:
#if defined(RWS_OS_WINDOWS)
    error_number = WSAGetLastError();
#else
    error_number = errno;
#endif

    if (error_number != WSAEWOULDBLOCK && error_number != WSAEINPROGRESS && error_number != 0)
    {
        LOG_W("[T:%.8d, L:%.5d, %*.*s] current error_number = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, error_number);

        LOG_E("[T:%.8d, L:%.5d, %*.*s] recv data failed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

        s->error = rws_error_new_code_descr(rws_error_code_read_write_socket, "Failed read/write socket");
        // rws_socket_close(s);

        result = rws_false;
    }
    else
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] recv data complete", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    }

_exit:
    return result;
}

_rws_frame *rws_socket_last_unfin_recvd_frame_by_opcode(_rws_socket *s, const rws_opcode opcode)
{
    _rws_frame *last = NULL;
    _rws_frame *frame = NULL;
    _rws_node *cur = s->recvd_frames;
    while (cur)
    {
        frame = (_rws_frame *)cur->value.object;
        if (frame)
        {
            //  [FIN=0,opcode !=0 ],[FIN=0,opcode ==0 ],....[FIN=1,opcode ==0 ]
            if (!frame->is_finished /*&& frame->opcode == opcode*/)
            {
                last = frame;
            }
        }
        cur = cur->next;
    }
    return last;
}

void rws_socket_process_bin_or_text_frame(_rws_socket *s, _rws_frame *frame)
{
    _rws_frame *last_unfin = rws_socket_last_unfin_recvd_frame_by_opcode(s, frame->opcode);
    if (last_unfin)
    {
        rws_frame_combine_datas(last_unfin, frame);
        last_unfin->is_finished = frame->is_finished;
        rws_frame_delete(frame);
    }
    else if (frame->data && frame->data_size)
    {
        rws_socket_append_recvd_frames(s, frame);
    }
    else
    {
        rws_frame_delete(frame);
    }
}

void rws_socket_process_ping_frame(_rws_socket *s, _rws_frame *frame)
{
    _rws_frame *pong_frame = rws_frame_create();
    pong_frame->opcode = rws_opcode_pong;
    pong_frame->is_masked = rws_true;
    rws_frame_fill_with_send_data(pong_frame, frame->data, frame->data_size);
    rws_frame_delete(frame);
    rws_socket_append_send_frames(s, pong_frame);
}

void rws_socket_process_conn_close_frame(_rws_socket *s, _rws_frame *frame)
{
    s->command = COMMAND_INFORM_DISCONNECTED;
    s->error = rws_error_new_code_descr(rws_error_code_connection_closed, "Connection was closed by endpoint");
    //rws_socket_close(s);
    rws_frame_delete(frame);
}

void rws_socket_process_received_frame(_rws_socket *s, _rws_frame *frame)
{
    switch (frame->opcode)
    {
    case rws_opcode_ping:
        rws_socket_process_ping_frame(s, frame);
        break;
    case rws_opcode_text_frame:
    case rws_opcode_binary_frame:
    case rws_opcode_continuation:
        rws_socket_process_bin_or_text_frame(s, frame);
        break;
    case rws_opcode_connection_close:
        rws_socket_process_conn_close_frame(s, frame);
        break;
    default:
        // unprocessed => delete
        rws_frame_delete(frame);
        break;
    }
}

void rws_socket_idle_recv(_rws_socket *s)
{
    _rws_frame *frame = NULL;

    if (s->custom_mode != 0x1234)
    {
        if (!rws_socket_recv(s))
        {
            // sock already closed
            if (s->error)
            {
                s->command = COMMAND_INFORM_DISCONNECTED;
            }
            return;
        }

        const size_t nframe_size = rws_check_recv_frame_size(s->received, s->received_len);
        if (nframe_size)
        {
            frame = rws_frame_create_with_recv_data(s->received, nframe_size);
            if (frame)
            {
                rws_socket_process_received_frame(s, frame);
            }

            if (nframe_size == s->received_len)
            {
                s->received_len = 0;
            }
            else if (s->received_len > nframe_size)
            {
                const size_t nLeftLen = s->received_len - nframe_size;
                memmove((char *)s->received, (char *)s->received + nframe_size, nLeftLen);
                s->received_len = nLeftLen;
            }
        }
    }

    /* RT-Thread Team add */
    else
    {
        if (!rws_socket_recv_custom(s))
        {
            // sock already closed, Todo
            if (s->error)
            {
                s->command = COMMAND_INFORM_DISCONNECTED;
            }
            return;
        }
    }
}

void rws_socket_idle_send(_rws_socket *s)
{
    _rws_node *cur = NULL;
    rws_bool sending = rws_true;
    _rws_frame *frame = NULL;

    rws_mutex_lock(s->send_mutex);
    cur = s->send_frames;
    if (cur)
    {
        while (cur && s->is_connected && sending)
        {
            frame = (_rws_frame *)cur->value.object;
            cur->value.object = NULL;
            if (frame)
            {
                sending = rws_socket_send(s, frame->data, frame->data_size);
            }
            rws_frame_delete(frame);
            cur = cur->next;
        }
        rws_list_delete_clean(&s->send_frames);
        if (s->error)
        {
            s->command = COMMAND_INFORM_DISCONNECTED;
        }
    }
    rws_mutex_unlock(s->send_mutex);
}

void rws_socket_wait_handshake_responce(_rws_socket *s)
{
    LOG_I("[T:%.8d, L:%.5d, %*.*s] try wait recv handshake responce", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

    /* Todo */
    if (!rws_socket_recv(s))
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] no find handshake responce", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

        if (s->error)
        {
            LOG_E("[T:%.8d, L:%.5d, %*.*s] recv responce failed, handshake failed, error = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->error);
            LOG_I("[T:%.8d, L:%.5d, %*.*s] try inform websocket disconnect", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            s->command = COMMAND_INFORM_DISCONNECTED;
        }

        return;
    }

    if (s->received_len == 0)
    {
        LOG_D("[T:%.8d, L:%.5d, %*.*s] handshake responce len is empty, len = 0", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        return;
    }

    LOG_I("[T:%.8d, L:%.5d, %*.*s] try process handshake responce", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    if (rws_socket_process_handshake_responce(s))
    {
        s->received_len = 0;
        s->is_connected = rws_true;
        s->command = COMMAND_INFORM_CONNECTED;

        LOG_D("[T:%.8d, L:%.5d, %*.*s] process handshake responce succeed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        LOG_I("[T:%.8d, L:%.5d, %*.*s] try inform websocket connect", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    }
    else
    {
        rws_socket_close(s);
        s->command = COMMAND_INFORM_DISCONNECTED;

        LOG_E("[T:%.8d, L:%.5d, %*.*s] process handshake responce failed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        LOG_I("[T:%.8d, L:%.5d, %*.*s] try inform websocket disconnect", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    }
}

void rws_socket_send_disconnect(_rws_socket *s)
{
    char buff[16];
    size_t len = 0;
    _rws_frame *frame = rws_frame_create();

    len = rws_sprintf(buff, 16, "%u", rws_socket_get_next_message_id(s));

    frame->is_masked = rws_true;
    frame->opcode = rws_opcode_connection_close;
    rws_frame_fill_with_send_data(frame, buff, len);
    rws_socket_send(s, frame->data, frame->data_size);
    rws_frame_delete(frame);
    s->command = COMMAND_END;
    rws_thread_sleep(RWS_CONNECT_RETRY_DELAY); // little bit wait after send message
}

void rws_socket_send_handshake(_rws_socket *s)
{
    char buff[512];
    char *ptr = buff;
    size_t writed = 0;
    writed = rws_sprintf(ptr, 512, "GET %s HTTP/%s\r\n", s->path, k_rws_socket_min_http_ver);

    if (s->port == 80)
    {
        writed += rws_sprintf(ptr + writed, 512 - writed, "Host: %s\r\n", s->host);
    }
    else
    {
        writed += rws_sprintf(ptr + writed, 512 - writed, "Host: %s:%i\r\n", s->host, s->port);
    }

    writed += rws_sprintf(ptr + writed, 512 - writed,
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Origin: %s://%s\r\n",
                          s->scheme, s->host);

    writed += rws_sprintf(ptr + writed, 512 - writed,
                          "Sec-WebSocket-Key: %s\r\n"
                          "Sec-WebSocket-Protocol: chat, superchat\r\n"
                          "Sec-WebSocket-Version: 13\r\n"
                          "\r\n",
                          "dGhlIHNhbXBsZSBub25jZQ==");

    if (rws_socket_send(s, buff, writed))
    {
        s->command = COMMAND_WAIT_HANDSHAKE_RESPONCE;
    }
    else
    {
        if (s->error)
        {
            s->error->code = rws_error_code_send_handshake;
        }
        else
        {
            s->error = rws_error_new_code_descr(rws_error_code_send_handshake, "Send handshake");
        }
        rws_socket_close(s);
        s->command = COMMAND_INFORM_DISCONNECTED;
    }
}

struct addrinfo *rws_socket_connect_getaddr_info(_rws_socket *s)
{
    struct addrinfo hints;
    char portstr[16];
    struct addrinfo *result = NULL;
    int ret = 0, retry_number = 0;
#if defined(RWS_OS_WINDOWS)
    WSADATA wsa;
#endif

    rws_error_delete_clean(&s->error);

#if defined(RWS_OS_WINDOWS)
    memset(&wsa, 0, sizeof(WSADATA));
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        s->error = rws_error_new_code_descr(rws_error_code_connect_to_host, "Failed initialise winsock");
        s->command = COMMAND_INFORM_DISCONNECTED;
        return NULL;
    }
#endif

    rws_sprintf(portstr, 16, "%i", s->port);

    while (++retry_number < RWS_CONNECT_ATTEMPS)
    {
        result = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        LOG_D("[T:%.8d, L:%.5d, %*.*s] port is %s", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, portstr);
        LOG_D("[T:%.8d, L:%.5d, %*.*s] retry_number is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, retry_number);

        ret = getaddrinfo(s->host, portstr, &hints, &result);

        LOG_D("[T:%.8d, L:%.5d, %*.*s] getaddrinfo ret: %d result:0x%08X", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, ret, result);

        if (ret == 0 && result)
        {
            return result;
        }

        if (result)
        {
            freeaddrinfo(result);
        }

        rws_thread_sleep(RWS_CONNECT_RETRY_DELAY);
    }

#if defined(RWS_OS_WINDOWS)
    WSACleanup();
#endif

    s->error = rws_error_new_code_descr(rws_error_code_connect_to_host, "Failed connect to host");
    // s->error = rws_error_new_code_descr(rws_error_code_connect_to_host,
    //                                     (last_ret > 0) ? gai_strerror(last_ret) : "Failed connect to host");

    s->command = COMMAND_INFORM_DISCONNECTED;
    LOG_I("[T:%.8d, L:%.5d, %*.*s] try inform websocket disconnect", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

    return NULL;
}

void rws_socket_connect_to_host(_rws_socket *s)
{
    struct addrinfo *result = NULL;
    struct addrinfo *p = NULL;
    rws_socket_t sock = RWS_INVALID_SOCKET;
    int retry_number = 0;
#if defined(RWS_OS_WINDOWS)
    unsigned long iMode = 0;
#endif

    result = rws_socket_connect_getaddr_info(s);
    if (!result)
    {
        LOG_E("[T:%.8d, L:%.5d, %*.*s] websocket connect failed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
        return;
    }

    while ((++retry_number < RWS_CONNECT_ATTEMPS) && (sock == RWS_INVALID_SOCKET))
    {
        LOG_I("[T:%.8d, L:%.5d, %*.*s] retry_number is %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, retry_number);

        for (p = result; p != NULL; p = p->ai_next)
        {
            LOG_I("[T:%.8d, L:%.5d, %*.*s] socket to: ai_family = %d, ai_socktype = %d, ai_protocol = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, p->ai_family, p->ai_socktype, p->ai_protocol);

            sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock != RWS_INVALID_SOCKET)
            {
                rws_socket_set_option(sock, SO_ERROR, 1);     // When an error occurs on a socket, set error variable so_error and notify process
                rws_socket_set_option(sock, SO_KEEPALIVE, 1); // Periodically test if connection is alive

                {
                    struct timeval timeout;

                    timeout.tv_sec  = 1;
                    timeout.tv_usec = 0;
                    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));
                    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
                }

                LOG_I("[T:%.8d, L:%.5d, %*.*s] connect to: ai_add = 0x%08X, ai_addrlen = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, (int)p->ai_addr, p->ai_addrlen);

                if (connect(sock, p->ai_addr, p->ai_addrlen) == 0)
                {
                    s->received_len = 0;
                    s->socket = sock;
#if defined(RWS_OS_WINDOWS)
                    // If iMode != 0, non-blocking mode is enabled.
                    iMode = 1;
                    ioctlsocket(s->socket, FIONBIO, &iMode);
#else
                    // fcntl(s->socket, F_SETFL, O_NONBLOCK);

                    {
                        struct timeval timeout;

                        timeout.tv_sec  = 3;
                        timeout.tv_usec = 0;
                        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));

                        timeout.tv_sec  = 0;
                        timeout.tv_usec = 1000;
                        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
                    }
#endif
                    LOG_I("[T:%.8d, L:%.5d, %*.*s] socket break", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

                    break;
                }

                LOG_E("[T:%.8d, L:%.5d, %*.*s] socket connect failed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                RWS_SOCK_CLOSE(sock);
            }
            else
            {
                LOG_I("[T:%.8d, L:%.5d, %*.*s] socket create failed", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }

        if (sock == RWS_INVALID_SOCKET)
        {
            rws_thread_sleep(RWS_CONNECT_RETRY_DELAY);
        }
    }

    freeaddrinfo(result);

    if (s->socket == RWS_INVALID_SOCKET)
    {
#if defined(RWS_OS_WINDOWS)
        WSACleanup();
#endif
        s->error = rws_error_new_code_descr(rws_error_code_connect_to_host, "Failed connect to host");
        s->command = COMMAND_INFORM_DISCONNECTED;

        LOG_I("[T:%.8d, L:%.5d, %*.*s] try inform websocket disconnect", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    }
    else
    {
        s->command = COMMAND_SEND_HANDSHAKE;

        LOG_I("[T:%.8d, L:%.5d, %*.*s] try send handshake", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
    }
}

static void rws_socket_work_th_func(void *user_object)
{
    _rws_socket *s = (_rws_socket *)user_object;
    size_t loop_number = 0;

    LOG_I("[T:%.8d, L:%.5d, %*.*s] this websocket work host is %s//%s:%d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->scheme, s->host, s->port);

    while (s->command < COMMAND_END)
    {
        loop_number++;

        rws_mutex_lock(s->work_mutex);

        switch (s->command)
        {
        case COMMAND_CONNECT_TO_HOST:
            LOG_I("[T:%.8d, L:%.5d, %*.*s] try connect to %s//%s:%d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->scheme, s->host, s->port);

#ifdef LIBRWS_USING_MBED_TLS
            if (s->scheme && strcmp(s->scheme, "wss") == 0)
            {
#if !defined(RWS_OS_WINDOWS)
                rws_ssl_conn(s);
#else
                LOG_E("[T:%.8d, L:%.5d, %*.*s] no support window", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

                s->socket == RWS_INVALID_SOCKET;
                s->command = COMMAND_INFORM_DISCONNECTED;
#endif
                break;
            }
            else
#endif /* LIBRWS_USING_MBED_TLS */
            {
                rws_socket_connect_to_host(s);
            }

            LOG_I("[T:%.8d, L:%.5d, %*.*s] try connect to %s//%s:%d end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->scheme, s->host, s->port);

            break;

        case COMMAND_SEND_HANDSHAKE:
            LOG_I("[T:%.8d, L:%.5d, %*.*s] COMMAND_SEND_HANDSHAKE", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            rws_socket_send_handshake(s);
            LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_SEND_HANDSHAKE end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            break;

        case COMMAND_WAIT_HANDSHAKE_RESPONCE:
            LOG_I("[T:%.8d, L:%.5d, %*.*s] COMMAND_WAIT_HANDSHAKE_RESPONCE", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            rws_socket_wait_handshake_responce(s);
            LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_WAIT_HANDSHAKE_RESPONCE end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            break;

        case COMMAND_DISCONNECT:
            LOG_I("[T:%.8d, L:%.5d, %*.*s] COMMAND_DISCONNECT", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            rws_socket_send_disconnect(s);
            LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_DISCONNECT end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            break;

        case COMMAND_IDLE:
            // LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_IDLE, loop = %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, loop_number);
            if (loop_number >= 400) // 400 4s 3 = 12s
            {
                loop_number = 0;

                if (s->is_connected)
                {
                    LOG_I("[T:%.8d, L:%.5d, %*.*s] send ping frame", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                    rws_socket_send_ping(s);
                    // LOG_I("[T:%.8d, L:%.5d, %*.*s] send ping frame end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                }
            }

            if (s->is_connected)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] call rws_socket_idle_send()", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                rws_socket_idle_send(s);
            }

            if (s->is_connected)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] call rws_socket_idle_recv()", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                rws_socket_idle_recv(s);
            }
            break;

        default:
            LOG_D("[T:%.8d, L:%.5d, %*.*s] other command: %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->command);
            break;
        }

        rws_mutex_unlock(s->work_mutex);

        switch (s->command)
        {
        case COMMAND_INFORM_CONNECTED:
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_INFORM_CONNECTED", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

            s->command = COMMAND_IDLE;
            if (s->on_connected)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] call on_connected function", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                s->on_connected(s);
                LOG_D("[T:%.8d, L:%.5d, %*.*s] call on_connected function end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }
        break;

        /* 断开连接需要关掉线程吗？ */
        case COMMAND_INFORM_DISCONNECTED:
        {
            LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_INFORM_DISCONNECTED", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

            s->command = COMMAND_END;
            rws_socket_send_disconnect(s);

            if (s->on_disconnected)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] call on_disconnected function", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                s->on_disconnected(s);
                LOG_D("[T:%.8d, L:%.5d, %*.*s] call on_disconnected function end", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
            }
        }
        break;

        case COMMAND_IDLE:
            LOG_D("[T:%.8d, L:%.5d, %*.*s] COMMAND_IDLE", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

            if (s->recvd_frames)
            {
                LOG_D("[T:%.8d, L:%.5d, %*.*s] rws_socket_inform_recvd_frames", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);
                rws_socket_inform_recvd_frames(s);
            }
            break;

        default:
            LOG_D("[T:%.8d, L:%.5d, %*.*s] other command: %d", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__, s->command);
            break;
        }

        rws_thread_sleep(5);
    }

    LOG_D("[T:%.8d, L:%.5d, %*.*s] this websocket work ready to quit, close socket", rt_tick_get(), __LINE__, DBG_FUNCTION_NUM, DBG_FUNCTION_NUM, __FUNCTION__);

    rws_socket_close(s);
    s->work_thread = NULL;
    rws_socket_delete(s);

    pthread_exit(NULL);
}

rws_bool rws_socket_create_start_work_thread(_rws_socket *s)
{
    rws_error_delete_clean(&s->error);
    s->command = COMMAND_NONE;
    s->work_thread = rws_thread_create(&rws_socket_work_th_func, s);
    if (s->work_thread)
    {
        s->command = COMMAND_CONNECT_TO_HOST;
        return rws_true;
    }
    return rws_false;
}

void rws_socket_resize_received(_rws_socket *s, const size_t size)
{
    void *res = NULL;
    size_t min = 0;
    if (size == s->received_size)
    {
        return;
    }

    res = rws_malloc(size);
    assert(res && (size > 0));

    min = (s->received_size < size) ? s->received_size : size;
    if (min > 0 && s->received)
    {
        memcpy(res, s->received, min);
    }
    rws_free_clean(&s->received);
    s->received = res;
    s->received_size = size;
}

void rws_socket_close(_rws_socket *s)
{
    s->received_len = 0;
    if (s->socket != RWS_INVALID_SOCKET)
    {
#ifdef LIBRWS_USING_MBED_TLS
        if (s->scheme && strcmp(s->scheme, "wss") == 0)
        {
            rws_ssl_close(s);
        }
        else
#endif /* LIBRWS_USING_MBED_TLS */
        {
            RWS_SOCK_CLOSE(s->socket);
        }

        s->socket = RWS_INVALID_SOCKET;
#if defined(RWS_OS_WINDOWS)
        WSACleanup();
#endif
    }
    s->is_connected = rws_false;
}

void rws_socket_append_recvd_frames(_rws_socket *s, _rws_frame *frame)
{
    _rws_node_value frame_list_var;
    frame_list_var.object = frame;
    if (s->recvd_frames)
    {
        rws_list_append(s->recvd_frames, frame_list_var);
    }
    else
    {
        s->recvd_frames = rws_list_create();
        s->recvd_frames->value = frame_list_var;
    }
}

void rws_socket_append_send_frames(_rws_socket *s, _rws_frame *frame)
{
    _rws_node_value frame_list_var;
    frame_list_var.object = frame;
    if (s->send_frames)
    {
        rws_list_append(s->send_frames, frame_list_var);
    }
    else
    {
        s->send_frames = rws_list_create();
        s->send_frames->value = frame_list_var;
    }
}

rws_bool rws_socket_send_text_priv(_rws_socket *s, const char *text)
{
    size_t len = text ? strlen(text) : 0;
    _rws_frame *frame = NULL;

    if (len <= 0)
    {
        return rws_false;
    }

    frame = rws_frame_create();
    frame->is_masked = rws_true;
    frame->opcode = rws_opcode_text_frame;
    rws_frame_fill_with_send_data(frame, text, len);
    rws_socket_append_send_frames(s, frame);

    return rws_true;
}

rws_bool rws_socket_send_bin_priv(_rws_socket *s, const char *data, size_t len, rws_opcode opcode, rws_bool is_fin)
{
    _rws_frame *frame = NULL;

    if (len <= 0) len = 0;

    frame = rws_frame_create();
    frame->is_masked = rws_true;
    frame->opcode = opcode;
    rws_frame_fill_with_send_bin(frame, data, len, is_fin);
    rws_socket_append_send_frames(s, frame);

    return rws_true;
}

void rws_socket_delete_all_frames_in_list(_rws_list *list_with_frames)
{
    _rws_frame *frame = NULL;
    _rws_node *cur = list_with_frames;
    while (cur)
    {
        frame = (_rws_frame *)cur->value.object;
        if (frame)
        {
            rws_frame_delete(frame);
        }
        cur->value.object = NULL;
    }
}

void rws_socket_set_option(rws_socket_t s, int option, int value)
{

    setsockopt(s, SOL_SOCKET, option, (char *)&value, sizeof(int));
}

void rws_socket_check_write_error(_rws_socket *s, int error_num)
{
#if defined(RWS_OS_WINDOWS)
    int socket_code = 0, code = 0;
    unsigned int socket_code_size = sizeof(int);
#else
    int socket_code = 0, code = 0;
    socklen_t socket_code_size = sizeof(socket_code);
#endif

    if (s->socket != RWS_INVALID_SOCKET)
    {
#if defined(RWS_OS_WINDOWS)
        if (getsockopt(s->socket, SOL_SOCKET, SO_ERROR, (char *)&socket_code, (int *)&socket_code_size) != 0)
        {
            socket_code = 0;
        }
#else
        if (getsockopt(s->socket, SOL_SOCKET, SO_ERROR, &socket_code, &socket_code_size) != 0)
        {
            socket_code = 0;
        }
#endif
    }

    code = (socket_code > 0) ? socket_code : error_num;
    if (code <= 0)
    {
        return;
    }

    switch (code)
    {
    // send errors
    case EACCES: //

    //		case EAGAIN: // The socket is marked nonblocking and the requested operation would block
    //		case EWOULDBLOCK: // The socket is marked nonblocking and the receive operation would block

    case EBADF:        // An invalid descriptor was specified
    case ECONNRESET:   // Connection reset by peer
    case EDESTADDRREQ: // The socket is not connection-mode, and no peer address is set
    case EFAULT:       // An invalid user space address was specified for an argument
    // The receive buffer pointer(s) point outside the process's address space.
    case EINTR:        // A signal occurred before any data was transmitted
    // The receive was interrupted by delivery of a signal before any data were available
    case EINVAL:       // Invalid argument passed
    case EISCONN:      // The connection-mode socket was connected already but a recipient was specified
    case EMSGSIZE:     // The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible
    case ENOBUFS:      // The output queue for a network interface was full
    case ENOMEM:       // No memory available
    case ENOTCONN:     // The socket is not connected, and no target has been given
    // The socket is associated with a connection-oriented protocol and has not been connected
    case ENOTSOCK:     // The argument sockfd is not a socket
    // The argument sockfd does not refer to a socket
    case EOPNOTSUPP:   // Some bit in the flags argument is inappropriate for the socket type.
    case EPIPE:        // The local end has been shut down on a connection oriented socket
    // recv errors
    case ECONNREFUSED: // A remote host refused to allow the network connection (typically because it is not running the requested service).

        s->error = rws_error_new_code_descr(rws_error_code_read_write_socket, rws_strerror(code));
        break;

    default:
        break;
    }
}

#ifdef LIBRWS_USING_MBED_TLS
#if defined(MBEDTLS_DEBUG_C)
#define DEBUG_LEVEL 1
#endif

void rws_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    LOG_I("%s", str ? str : NULL);
}

// TODO: mbedtls_ssl_set_hostname() should be called.
int rws_ssl_conn(_rws_socket *s)
{
    int authmode = MBEDTLS_SSL_VERIFY_NONE;
    const char *pers = "https";
    int value, ret = 0;
    uint32_t flags;
    _rws_ssl *ssl = NULL;
    char portstr[16] = {0};

    value = value;

    s->ssl = rws_malloc_zero(sizeof(_rws_ssl));
    if (!s->ssl)
    {
        LOG_I("Memory malloc error.");
        ret = -1;
        goto exit;
    }
    ssl = s->ssl;

    if (s->server_cert)
        authmode = MBEDTLS_SSL_VERIFY_OPTIONAL;

    /*
    * Initialize the RNG and the session data
    */
#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif
    mbedtls_net_init(&ssl->net_ctx);
    mbedtls_ssl_init(&ssl->ssl_ctx);
    mbedtls_ssl_config_init(&ssl->ssl_conf);
    mbedtls_x509_crt_init(&ssl->cacert);
    mbedtls_x509_crt_init(&ssl->clicert);
    mbedtls_pk_init(&ssl->pkey);
    mbedtls_ctr_drbg_init(&ssl->ctr_drbg);

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_entropy_init(&ssl->entropy);
    if ((value = mbedtls_ctr_drbg_seed(&ssl->ctr_drbg,
                                       mbedtls_entropy_func,
                                       &ssl->entropy,
                                       (const unsigned char *)pers,
                                       strlen(pers))) != 0)
    {
        LOG_I("mbedtls_ctr_drbg_seed() failed, value:-0x%x.", -value);
        ret = -1;
        goto exit;
    }

    /*
    * Load the Client certificate
    */
    if (s->client_cert && s->client_pk)
    {
        ret = mbedtls_x509_crt_parse(&ssl->clicert, (const unsigned char *)s->client_cert, s->client_cert_len);
        if (ret < 0)
        {
            LOG_I("Loading cli_cert failed! mbedtls_x509_crt_parse returned -0x%x.", -ret);
            goto exit;
        }

        ret = mbedtls_pk_parse_key(&ssl->pkey, (const unsigned char *)s->client_pk, s->client_pk_len, NULL, 0);
        if (ret != 0)
        {
            LOG_I("failed! mbedtls_pk_parse_key returned -0x%x.", -ret);
            goto exit;
        }
    }

    /*
    * Load the trusted CA
    */
    /* cert_len passed in is gotten from sizeof not strlen */
    if (s->server_cert && ((value = mbedtls_x509_crt_parse(&ssl->cacert,
                                    (const unsigned char *)s->server_cert,
                                    s->server_cert_len)) < 0))
    {
        LOG_I("mbedtls_x509_crt_parse() failed, value:-0x%x.", -value);
        ret = -1;
        goto exit;
    }

    // rws_socket_set_option(ssl->net_ctx.fd, SO_ERROR, 1);     // When an error occurs on a socket, set error variable so_error and notify process
    // rws_socket_set_option(ssl->net_ctx.fd, SO_KEEPALIVE, 1); // Periodically test if connection is alive

    // {
    //     struct timeval timeout;

    //     timeout.tv_sec  = 1;
    //     timeout.tv_usec = 0;
    //     setsockopt(ssl->net_ctx.fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));
    //     setsockopt(ssl->net_ctx.fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
    // }

    /*
     * Start the connection
     */
    rws_sprintf(portstr, 16, "%i", s->port);
    if ((ret = mbedtls_net_connect(&ssl->net_ctx, s->host, portstr, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        LOG_I("failed! mbedtls_net_connect returned %d, port:%s.", ret, portstr);
        goto exit;
    }

    s->received_len = 0;
    s->socket = ssl->net_ctx.fd;

    /*
     * Setup stuff
     */
    if ((value = mbedtls_ssl_config_defaults(&ssl->ssl_conf,
                 MBEDTLS_SSL_IS_CLIENT,
                 MBEDTLS_SSL_TRANSPORT_STREAM,
                 MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        LOG_I("mbedtls_ssl_config_defaults() failed, value:-0x%x.", -value);
        ret = -1;
        goto exit;
    }

    // TODO: add customerization encryption algorithm
    memcpy(&ssl->profile, ssl->ssl_conf.cert_profile, sizeof(mbedtls_x509_crt_profile));
    ssl->profile.allowed_mds = ssl->profile.allowed_mds | MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_MD5);
    mbedtls_ssl_conf_cert_profile(&ssl->ssl_conf, &ssl->profile);

    mbedtls_ssl_conf_authmode(&ssl->ssl_conf, authmode);
    mbedtls_ssl_conf_ca_chain(&ssl->ssl_conf, &ssl->cacert, NULL);
    mbedtls_ssl_conf_max_frag_len(&ssl->ssl_conf, MBEDTLS_SSL_MAX_FRAG_LEN_4096);

    if (s->client_cert && (ret = mbedtls_ssl_conf_own_cert(&ssl->ssl_conf, &ssl->clicert, &ssl->pkey)) != 0)
    {
        LOG_I(" failed! mbedtls_ssl_conf_own_cert returned %d.", ret);
        goto exit;
    }

    mbedtls_ssl_conf_rng(&ssl->ssl_conf, mbedtls_ctr_drbg_random, &ssl->ctr_drbg);
    mbedtls_ssl_conf_dbg(&ssl->ssl_conf, rws_debug, NULL);

    if ((value = mbedtls_ssl_setup(&ssl->ssl_ctx, &ssl->ssl_conf)) != 0)
    {
        LOG_I("mbedtls_ssl_setup() failed, value:-0x%x.", -value);
        ret = -1;
        goto exit;
    }

    if ((ret = mbedtls_ssl_set_hostname(&ssl->ssl_ctx, s->host)) != 0)
    {
        LOG_I("failed\n! mbedtls_ssl_set_hostname returned %d\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&ssl->ssl_ctx, &ssl->net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

    /*
    * Handshake
    */
    while ((ret = mbedtls_ssl_handshake(&ssl->ssl_ctx)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            LOG_I("mbedtls_ssl_handshake() failed, ret:-0x%x.", -ret);
            ret = -1;
            goto exit;
        }

        LOG_I("mbedtls_ssl_handshake() while");
    }

    LOG_I("mbedtls_ssl_handshake() ok");

    // Todo
    // mbedtls_net_set_nonblock(&ssl->net_ctx);

    {
        struct timeval timeout;

        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;
        setsockopt(ssl->net_ctx.fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(timeout));

        timeout.tv_sec  = 0;
        timeout.tv_usec = 1000;
        setsockopt(ssl->net_ctx.fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));
    }

    /*
     * Verify the server certificate
     */
    /* In real life, we would have used MBEDTLS_SSL_VERIFY_REQUIRED so that the
        * handshake would not succeed if the peer's cert is bad.  Even if we used
        * MBEDTLS_SSL_VERIFY_OPTIONAL, we would bail out here if ret != 0 */
    if ((flags = mbedtls_ssl_get_verify_result(&ssl->ssl_ctx)) != 0)
    {
        char vrfy_buf[512];
        LOG_I("svr_cert varification failed. authmode:%d", authmode);
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
        LOG_I("%s", vrfy_buf);
    }
    else
    {
        LOG_I("svr_cert varification ok. authmode:%d", authmode);
    }

exit:
    if (ret != 0)
    {
#ifdef MBEDTLS_ERROR_C
        char error_buf[100];

        extern void mbedtls_strerror(int ret, char *buf, size_t buflen);
        mbedtls_strerror(ret, error_buf, 100);
        LOG_I("Last error was: %d - %s", ret, error_buf);
#endif
        LOG_I("ret=%d.", ret);
        s->error = rws_error_new_code_descr(rws_error_code_connect_to_host, "Failed connect to host");
        LOG_I("%s: code:%d, error:%s", __FUNCTION__, s->error->code, s->error->description);
        s->command = COMMAND_INFORM_DISCONNECTED;

        if (s->ssl)
        {
            mbedtls_net_free(&s->ssl->net_ctx);
            mbedtls_x509_crt_free(&s->ssl->cacert);
            mbedtls_x509_crt_free(&s->ssl->clicert);
            mbedtls_pk_free(&s->ssl->pkey);
            mbedtls_ssl_free(&s->ssl->ssl_ctx);
            mbedtls_ssl_config_free(&s->ssl->ssl_conf);
            mbedtls_ctr_drbg_free(&s->ssl->ctr_drbg);
            mbedtls_entropy_free(&s->ssl->entropy);

            rws_free(s->ssl);
            s->ssl = NULL;
        }

        s->socket == RWS_INVALID_SOCKET;
    }
    else
    {
        s->command = COMMAND_SEND_HANDSHAKE;
    }
    return ret;
}

int rws_ssl_close(_rws_socket *s)
{
    _rws_ssl *ssl = s->ssl;

    if (!ssl)
        return -1;

    mbedtls_ssl_close_notify(&ssl->ssl_ctx);
    mbedtls_net_free(&ssl->net_ctx);
    mbedtls_x509_crt_free(&ssl->cacert);
    mbedtls_x509_crt_free(&ssl->clicert);
    mbedtls_pk_free(&ssl->pkey);
    mbedtls_ssl_free(&ssl->ssl_ctx);
    mbedtls_ssl_config_free(&ssl->ssl_conf);
    mbedtls_ctr_drbg_free(&ssl->ctr_drbg);
    mbedtls_entropy_free(&ssl->entropy);

    rws_free(ssl);
    s->ssl = NULL;

    return 0;
}
#endif
