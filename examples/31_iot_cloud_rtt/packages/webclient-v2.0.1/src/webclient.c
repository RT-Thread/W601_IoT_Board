/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-05-05     Bernard      the first version
 * 2013-06-10     Bernard      fix the slow speed issue when download file.
 * 2015-11-14     aozima       add content_length_remainder.
 * 2017-12-23     aozima       update gethostbyname to getaddrinfo.
 * 2018-01-04     aozima       add ipv6 address support.
 * 2018-07-26     chenyong     modify log information
 * 2018-08-07     chenyong     modify header processing
 */

#include <string.h>
#include <sys/time.h>

#include <webclient.h>

/* support both enable and disable "SAL_USING_POSIX" */
#if defined(RT_USING_SAL)
#include <netdb.h>
#include <sys/socket.h>
#else
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#endif /* RT_USING_SAL */

#define DBG_ENABLE
#define DBG_SECTION_NAME               "web"
#ifdef WEBCLIENT_DEBUG
#define DBG_LEVEL                      DBG_LOG
#else
#define DBG_LEVEL                      DBG_INFO
#endif /* WEBCLIENT_DEBUG */
#define DBG_COLOR
#include <rtdbg.h>

/* default receive or send timeout */
#define WEBCLIENT_DEFAULT_TIMEO        6

extern long int strtol(const char *nptr, char **endptr, int base);

static int webclient_send(struct webclient_session* session, const unsigned char *buffer, size_t len, int flag)
{
#ifdef WEBCLIENT_USING_TLS
    if(session->tls_session)
    {
        return mbedtls_client_write(session->tls_session, buffer, len);
    }
#endif

    return send(session->socket, buffer, len, flag);
}

static int webclient_recv(struct webclient_session* session, unsigned char *buffer, size_t len, int flag)
{
#ifdef WEBCLIENT_USING_TLS
    if(session->tls_session)
    {
        return mbedtls_client_read(session->tls_session, buffer, len);
    }
#endif 

    return recv(session->socket, buffer, len, flag);
}

static int webclient_read_line(struct webclient_session *session, char *buffer, int size)
{
    int rc, count = 0;
    char ch = 0, last_ch = 0;

    RT_ASSERT(session);
    RT_ASSERT(buffer);

    /* Keep reading until we fill the buffer. */
    while (count < size)
    {
        rc = webclient_recv(session, (unsigned char *) &ch, 1, 0);
#ifdef WEBCLIENT_USING_TLS
        if(session->tls_session && rc == MBEDTLS_ERR_SSL_WANT_READ)
            continue;
#endif 
        if (rc <= 0)
            return rc;

        if (ch == '\n' && last_ch == '\r')
            break;

        buffer[count++] = ch;

        last_ch = ch;
    }

    if (count > size)
    {
        LOG_E("read line failed. The line data length is out of buffer size(%d)!", count);
        return -WEBCLIENT_ERROR;
    }

    return count;
}

/**
 * resolve server address
 *
 * @param session http session
 * @param res the server address information
 * @param url the input server URI address
 * @param request the pointer to point the request url, for example, /index.html
 *
 * @return 0 on resolve server address OK, others failed
 *
 * URL example:
 * http://www.rt-thread.org/
 * http://192.168.1.1:80/index.htm
 * http://[fe80::1]/index.html
 * http://[fe80::1]:80/index.html
 */
static int webclient_resolve_address(struct webclient_session *session, struct addrinfo **res,
                                     const char *url, char **request)
{
    int rc = WEBCLIENT_OK;
    char *ptr;
    char port_str[6] = "80"; /* default port of 80(http) */

    const char *host_addr = 0;
    int url_len, host_addr_len = 0;

    RT_ASSERT(res);
    RT_ASSERT(request);

    url_len = rt_strlen(url);

    /* strip protocol(http or https) */
    if (strncmp(url, "http://", 7) == 0)
    {
        host_addr = url + 7;
    }
    else if (strncmp(url, "https://", 8) == 0)
    {
        rt_strncpy(port_str, "443", 4);
        host_addr = url + 8;
    }
    else
    {
        rc = -WEBCLIENT_ERROR;
        goto __exit;
    }

    /* ipv6 address */
    if (host_addr[0] == '[')
    {
        host_addr += 1;
        ptr = rt_strstr(host_addr, "]");
        if (!ptr)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
        host_addr_len = ptr - host_addr;

        ptr = rt_strstr(host_addr + host_addr_len, "/");
        if (!ptr)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
        else if (ptr != (host_addr + host_addr_len + 1))
        {
            int port_len = ptr - host_addr - host_addr_len - 2;

            rt_strncpy(port_str, host_addr + host_addr_len + 2, port_len);
            port_str[port_len] = '\0';
        }

        *request = (char *) ptr;
    }
    else /* ipv4 or domain. */
    {
        char *port_ptr;

        ptr = rt_strstr(host_addr, "/");
        if (!ptr)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
        host_addr_len = ptr - host_addr;
        *request = (char *) ptr;
        
        /* resolve port */
        port_ptr = rt_strstr(host_addr, ":");
        if (port_ptr && port_ptr < ptr)
        {
            int port_len = ptr - port_ptr - 1;

            rt_strncpy(port_str, port_ptr + 1, port_len);
            port_str[port_len] = '\0';

            host_addr_len = port_ptr - host_addr;
        }
    }

    if ((host_addr_len < 1) || (host_addr_len > url_len))
    {
        rc = -WEBCLIENT_ERROR;
        goto __exit;
    }

    /* get host address ok. */
    {
        char *host_addr_new = web_malloc(host_addr_len + 1);

        if (!host_addr_new)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }

        memcpy(host_addr_new, host_addr, host_addr_len);
        host_addr_new[host_addr_len] = '\0';
        session->host = host_addr_new;
        
#ifdef WEBCLIENT_USING_TLS
        if(session->tls_session)
            session->tls_session->host = web_strdup(host_addr_new);
#endif
    }

    LOG_D("host address: %s , port: %s", session->host, port_str);

    /* resolve the host name. */
    {
        struct addrinfo hint;
        int ret;

        rt_memset(&hint, 0, sizeof(hint));
        
#ifdef WEBCLIENT_USING_TLS
        if(session->tls_session)
        {
            session->tls_session->port = web_strdup(port_str);
            ret = getaddrinfo(session->tls_session->host, port_str, &hint, res);
            if (ret != 0)
            {
                LOG_E("getaddrinfo err: %d '%s'", ret, session->host);
                rc = -WEBCLIENT_ERROR;          
            }
            
            goto __exit;
        }
#endif

        ret = getaddrinfo(session->host, port_str, &hint, res);
        if (ret != 0)
        {
            LOG_E("getaddrinfo err: %d '%s'.", ret, session->host);
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }

    }
__exit:
    if (rc != WEBCLIENT_OK)
    {
        if (session->host)
        {
            web_free(session->host);
            session->host = RT_NULL;
        }

        if (*res)
        {
            freeaddrinfo(*res);
            *res = RT_NULL;
        }
    }

    return rc;
}

#ifdef WEBCLIENT_USING_TLS
/**
 * create and initialize https session.
 *
 * @param session webclient session
 * @param URI input server URI address
 *
 * @return <0: create failed, no memory or other errors
 *         =0: success
 */
static int webclient_open_tls(struct webclient_session *session, const char *URI)
{
    int tls_ret = 0;
    const char *pers = "webclient";

    RT_ASSERT(session);

    session->tls_session = (MbedTLSSession *) web_calloc(1, sizeof(MbedTLSSession));
    if (session->tls_session == RT_NULL)
    {
        return -WEBCLIENT_NOMEM;
    }

    session->tls_session->buffer_len = WEBCLIENT_RESPONSE_BUFSZ;
    session->tls_session->buffer = web_malloc(session->tls_session->buffer_len);
    if(session->tls_session->buffer == RT_NULL)
    {
        LOG_E("no memory for tls_session buffer!");
        return -WEBCLIENT_ERROR;
    }

    if((tls_ret = mbedtls_client_init(session->tls_session, (void *)pers, strlen(pers))) < 0)
    {
        LOG_E("initialize https client failed return: -0x%x.", -tls_ret);
        return -WEBCLIENT_ERROR;
    }

    return WEBCLIENT_OK;
}
#endif

/**
 * connect to http server.
 *
 * @param session webclient session
 * @param URI the input server URI address
 *
 * @return <0: connect failed or other error
 *         =0: connect success
 */
static int webclient_connect(struct webclient_session *session, const char *URI)
{
    int rc = WEBCLIENT_OK;
    int socket_handle;
    struct timeval timeout;
    struct addrinfo *res = RT_NULL;
    char *req_url;

    RT_ASSERT(session);
    RT_ASSERT(URI);

    /* initialize the socket of session */
    session->socket = -1;

    timeout.tv_sec = WEBCLIENT_DEFAULT_TIMEO;
    timeout.tv_usec = 0;

    if (strncmp(URI, "https://", 8) == 0)
    {
#ifdef WEBCLIENT_USING_TLS
        if(webclient_open_tls(session, URI) < 0)
        {
            LOG_E("connect failed, https client open URI(%s) failed!", URI);
            return -WEBCLIENT_ERROR;
        }
#else
        LOG_E("not support https connect, please enable webclient https configure!");
        rc = -WEBCLIENT_ERROR;
        goto __exit;
#endif
    }

    /* Check valid IP address and URL */
    rc = webclient_resolve_address(session, &res, URI, &req_url);
    if (rc != WEBCLIENT_OK)
    {
        LOG_E("connect failed, resolve address error(%d).", rc);
        goto __exit;
    }

    if (!res)
    {
        rc = -WEBCLIENT_ERROR;
        goto __exit;
    }

    /* copy host address */
    if (*req_url)
    {
        session->req_url = web_strdup(req_url);
    }

#ifdef WEBCLIENT_USING_TLS
    if (session->tls_session)
    {
        int tls_ret = 0;

        if ((tls_ret = mbedtls_client_context(session->tls_session)) < 0)
        {
            LOG_E("connect failed, https client context return: -0x%x", -tls_ret);
            return -WEBCLIENT_ERROR;
        }

        if ((tls_ret = mbedtls_client_connect(session->tls_session)) < 0)
        {
            LOG_E("connect failed, https client connect return: -0x%x", -tls_ret);
            rc = -WEBCLIENT_CONNECT_FAILED;
            goto __exit;
        }

        socket_handle = session->tls_session->server_fd.fd;

        /* set recv timeout option */
        setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, (void*) &timeout,
                sizeof(timeout));
        setsockopt(socket_handle, SOL_SOCKET, SO_SNDTIMEO, (void*) &timeout,
                sizeof(timeout));

        session->socket = socket_handle;

        rc = WEBCLIENT_OK;
        goto __exit;
    }
#endif

    {
        socket_handle = socket(res->ai_family, SOCK_STREAM, IPPROTO_TCP);
        if (socket_handle < 0)
        {
            LOG_E("connect failed, create socket(%d) error.", socket_handle);
            rc = -WEBCLIENT_NOSOCKET;
            goto __exit;
        }

        /* set receive and send timeout option */
        setsockopt(socket_handle, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeout,
                   sizeof(timeout));
        setsockopt(socket_handle, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeout,
                   sizeof(timeout));

        if (connect(socket_handle, res->ai_addr, res->ai_addrlen) != 0)
        {
            /* connect failed */
            LOG_E("connect failed, connect socket(%d) error.", socket_handle);
            rc = -WEBCLIENT_CONNECT_FAILED;
            goto __exit;
        }

        session->socket = socket_handle;
    }

__exit:
    if (res)
    {
        freeaddrinfo(res);
    }

    return rc;
}

/**
 * add fields data to request header data.
 *
 * @param session webclient session
 * @param fmt fields format
 *
 * @return >0: data length of successfully added
 *         <0: not enough header buffer size
 */
int webclient_header_fields_add(struct webclient_session *session, const char *fmt, ...)
{
    rt_int32_t length;
    va_list args;

    RT_ASSERT(session);
    RT_ASSERT(session->header->buffer);

    va_start(args, fmt);
    length = rt_vsnprintf(session->header->buffer + session->header->length,
            session->header->size - session->header->length, fmt, args);
    if (length < 0)
    {
        LOG_E("add fields header data failed, return length(%d) error.", length);
        va_end(args);
        return -WEBCLIENT_ERROR;
    }
    va_end(args);

    session->header->length += length;

    /* check header size */
    if (session->header->length >= session->header->size)
    {
        LOG_E("not enough header buffer size(%d)!", session->header->size);
        return -WEBCLIENT_ERROR;
    }

    return length;
}

/**
 * get fields information from request/response header data.
 *
 * @param session webclient session
 * @param fields fields keyword
 *
 * @return = NULL: get fields data failed
 *        != NULL: success get fields data
 */
const char *webclient_header_fields_get(struct webclient_session *session, const char *fields)
{
    char *resp_buf = RT_NULL;
    size_t resp_buf_len = 0;

    RT_ASSERT(session);
    RT_ASSERT(session->header->buffer);

    resp_buf = session->header->buffer;
    while (resp_buf_len < session->header->length)
    {
        if (rt_strstr(resp_buf, fields))
        {
            char *mime_ptr = RT_NULL;

            /* jump space */
            mime_ptr = rt_strstr(resp_buf, ":");
            if (mime_ptr != NULL)
            {
                mime_ptr += 1;

                while (*mime_ptr && (*mime_ptr == ' ' || *mime_ptr == '\t'))
                    mime_ptr++;

                return mime_ptr;
            }
        }

        if (*resp_buf == '\0')
            break;

        resp_buf += rt_strlen(resp_buf) + 1;
        resp_buf_len += rt_strlen(resp_buf) + 1;
    }

    return RT_NULL;
}

/**
 * get http response status code.
 *
 * @param session webclient session
 *
 * @return response status code
 */
int webclient_resp_status_get(struct webclient_session *session)
{
    RT_ASSERT(session);

    return session->resp_status;
}

/**
 * get http response data content length.
 *
 * @param session webclient session
 *
 * @return response content length
 */
int webclient_content_length_get(struct webclient_session *session)
{
    RT_ASSERT(session);

    return session->content_length;
}

static int webclient_send_header(struct webclient_session *session, int method)
{
    int rc = WEBCLIENT_OK;
    char *header = RT_NULL;

    RT_ASSERT(session);

    header = session->header->buffer;

    if (session->header->length == 0)
    {
        /* use default header data */
        if (webclient_header_fields_add(session, "GET %s HTTP/1.1\r\n", session->req_url) < 0)
            return -WEBCLIENT_NOMEM;
        if (webclient_header_fields_add(session, "Host: %s\r\n", session->host) < 0)
            return -WEBCLIENT_NOMEM;
        if (webclient_header_fields_add(session, "User-Agent: RT-Thread HTTP Agent\r\n\r\n") < 0)
            return -WEBCLIENT_NOMEM;

        webclient_write(session, (unsigned char *) session->header->buffer, session->header->length);
    }
    else
    {
        if (method != WEBCLIENT_USER_METHOD)
        {
            /* check and add fields header data */
            if (memcmp(header, "HTTP/1.", rt_strlen("HTTP/1.")))
            {
                char *header_buffer = RT_NULL;
                int length = 0;

                header_buffer = web_strdup(session->header->buffer);
                if (header_buffer == RT_NULL)
                {
                    LOG_E("no memory for header buffer!");
                    rc = -WEBCLIENT_NOMEM;
                    goto __exit;
                }

                /* splice http request header data */
                if (method == WEBCLIENT_GET)
                    length = rt_snprintf(session->header->buffer, session->header->size, "GET %s HTTP/1.1\r\n%s",
                            session->req_url ? session->req_url : "/", header_buffer);
                else if (method == WEBCLIENT_POST)
                    length = rt_snprintf(session->header->buffer, session->header->size, "POST %s HTTP/1.1\r\n%s",
                            session->req_url ? session->req_url : "/", header_buffer);
                session->header->length = length;

                web_free(header_buffer);
            }

            if (memcmp(header, "Host:", rt_strlen("Host:")))
            {
                if (webclient_header_fields_add(session, "Host: %s\r\n", session->host) < 0)
                    return -WEBCLIENT_NOMEM;
            }

            if (memcmp(header, "User-Agent:", rt_strlen("User-Agent:")))
            {
                if (webclient_header_fields_add(session, "User-Agent: RT-Thread HTTP Agent\r\n") < 0)
                    return -WEBCLIENT_NOMEM;
            }

            if (memcmp(header, "Accept:", rt_strlen("Accept:")))
            {
                if (webclient_header_fields_add(session, "Accept: */*\r\n") < 0)
                    return -WEBCLIENT_NOMEM;
            }

            /* header data end */
            rt_snprintf(session->header->buffer + session->header->length, session->header->size, "\r\n");
            session->header->length += 2;

            /* check header size */
            if (session->header->length > session->header->size)
            {
                LOG_E("send header failed, not enough header buffer size(%d)!", session->header->size);
                rc = -WEBCLIENT_NOBUFFER;
                goto __exit;
            }

            webclient_write(session, (unsigned char *) session->header->buffer, session->header->length);
        }
        else
        {
            webclient_write(session, (unsigned char *) session->header->buffer, session->header->length);
        }
    }

__exit:
    return rc;
}

/**
 * resolve server response data.
 *
 * @param session webclient session
 *
 * @return <0: resolve response data failed
 *         =0: success
 */
int webclient_handle_response(struct webclient_session *session)
{
    int rc = WEBCLIENT_OK;
    char *mime_buffer = RT_NULL;
    char *mime_ptr = RT_NULL;
    const char *transfer_encoding;
    int i;

    RT_ASSERT(session);

    /* clean header buffer and size */
    rt_memset(session->header->buffer, 0x00, session->header->size);
    session->header->length = 0;

    /* We now need to read the header information */
    while (1)
    {
        mime_buffer = session->header->buffer + session->header->length;

        /* read a line from the header information. */
        rc = webclient_read_line(session, mime_buffer, session->header->size - session->header->length);
        if (rc < 0)
            break;

        /* End of headers is a blank line.  exit. */
        if (rc == 0)
            break;
        if ((rc == 1) && (mime_buffer[0] == '\r'))
        {
            mime_buffer[0] = '\0';
            break;
        }

        /* set terminal charater */
        mime_buffer[rc - 1] = '\0';

        session->header->length += rc;

        if (session->header->length >= session->header->size)
        {
            LOG_E("not enough header buffer size(%d)!", session->header->size);
            return -WEBCLIENT_NOMEM;
        }
    }

    /* get HTTP status code */
    mime_ptr = web_strdup(session->header->buffer);
    if (mime_ptr == RT_NULL)
    {
        LOG_E("no memory for get http status code buffer!");
        return -WEBCLIENT_NOMEM;
    }

    if (rt_strstr(mime_ptr, "HTTP/1."))
    {
        char *ptr = mime_ptr;

        ptr += rt_strlen("HTTP/1.x");

        while (*ptr && (*ptr == ' ' || *ptr == '\t'))
            ptr++;

        /* Terminate string after status code */
        for (i = 0; ((ptr[i] != ' ') && (ptr[i] != '\t')); i++);
        ptr[i] = '\0';

        session->resp_status = (int) strtol(ptr, RT_NULL, 10);
    }

    /* get content length */
    if (webclient_header_fields_get(session, "Content-Length") != RT_NULL)
    {
        session->content_length = atoi(webclient_header_fields_get(session, "Content-Length"));
    }
    session->content_remainder = session->content_length ? (size_t) session->content_length : 0xFFFFFFFF;

    transfer_encoding = webclient_header_fields_get(session, "Transfer-Encoding");
    if (transfer_encoding && rt_strcmp(transfer_encoding, "chunked") == 0)
    {
        char line[16];

        /* chunk mode, we should get the first chunk size */
        webclient_read_line(session, line, session->header->size);
        session->chunk_sz = strtol(line, RT_NULL, 16);
        session->chunk_offset = 0;
    }

    if (mime_ptr)
    {
        web_free(mime_ptr);
    }

    if (rc < 0)
    {
        return rc;
    }

    return session->resp_status;
}

/**
 * create webclient session, set maximum header and response size
 *
 * @param header_sz maximum send header size
 * @param resp_sz maximum response data size
 *
 * @return  webclient session structure
 */
struct webclient_session *webclient_session_create(size_t header_sz)
{
    struct webclient_session *session;

    /* create session */
    session = (struct webclient_session *) web_calloc(1, sizeof(struct webclient_session));
    if (session == RT_NULL)
    {
        LOG_E("webclient create failed, no memory for webclient session!");
        return RT_NULL;
    }

    session->content_length = -1;

    session->header = (struct webclient_header *) web_calloc(1, sizeof(struct webclient_header));
    if (session->header == RT_NULL)
    {
        LOG_E("webclient create failed, no memory for session header!");
        web_free(session);
        session = RT_NULL;
        return RT_NULL;
    }

    session->header->size = header_sz;
    session->header->buffer = (char *) web_malloc(header_sz);
    if (session->header->buffer == RT_NULL)
    {
        LOG_E("webclient create failed, no memory for session header buffer!");
        web_free(session->header);
        web_free(session);
        session = RT_NULL;
        return RT_NULL;
    }

    return session;
}

/**
 *  send GET request to http server and get response header.
 *
 * @param session webclient session
 * @param URI input server URI address
 * @param header GET request header
 *             = NULL: use default header data
 *            != NULL: use custom header data
 *
 * @return <0: send GET request failed
 *         >0: response http status code
 */
int webclient_get(struct webclient_session *session, const char *URI)
{
    int rc = WEBCLIENT_OK;
    int resp_status = 0;

    RT_ASSERT(session);
    RT_ASSERT(URI);

    rc = webclient_connect(session, URI);
    if (rc != WEBCLIENT_OK)
    {
        /* connect to webclient server failed. */
        return rc;
    }

    rc = webclient_send_header(session, WEBCLIENT_GET);
    if (rc != WEBCLIENT_OK)
    {
        /* send header to webclient server failed. */
       return rc;
    }

    /* handle the response header of webclient server */
    resp_status = webclient_handle_response(session);
    if (resp_status > 0)
    {
        const char *location = webclient_header_fields_get(session, "Location");

        /* relocation */
        if ((resp_status == 302 || resp_status == 301) && location)
        {
            webclient_close(session);
            return webclient_get(session, location);
        }
        else if (resp_status != 200)
        {
            LOG_E("get failed, handle response(%d) error!", resp_status);
            return resp_status;
        }
    }

    return resp_status;
}

/**
 *  http breakpoint resume.
 *
 * @param session webclient session
 * @param URI input server URI address
 * @param position last downloaded position
 *
 * @return <0: send GET request failed
 *         >0: response http status code
 */
int webclient_get_position(struct webclient_session *session, const char *URI, int position)
{
    int rc = WEBCLIENT_OK;
    int resp_status = 0;

    RT_ASSERT(session);
    RT_ASSERT(URI);

    rc = webclient_connect(session, URI);
    if (rc != WEBCLIENT_OK)
    {
        return rc;
    }

    /* splice header*/
    if (webclient_header_fields_add(session, "Range: bytes=%d-\r\n", position) <= 0)
    {
        rc = -WEBCLIENT_ERROR;
        return rc;
    }

    rc = webclient_send_header(session, WEBCLIENT_GET);
    if (rc != WEBCLIENT_OK)
    {
        return rc;
    }

    /* handle the response header of webclient server */
    resp_status = webclient_handle_response(session);
    if (resp_status > 0)
    {
        const char *location = webclient_header_fields_get(session, "Location");

        /* relocation */
        if ((resp_status == 302 || resp_status == 301) && location)
        {
            webclient_close(session);
            return webclient_get_position(session, location, position);
        }
        else if (resp_status != 206)
        {
            LOG_E("get failed, handle response(%d) error!", resp_status);
            return resp_status;
        }
    }

    return resp_status;
}

/**
 * send POST request to server and get response header data.
 *
 * @param session webclient session
 * @param URI input server URI address
 * @param header POST request header, can't be empty
 * @param post_data data sent to the server
 *                = NULL: just connect server and send header
 *               != NULL: send header and body data, resolve response data
 *
 * @return <0: send POST request failed
 *         =0: send POST header success
 *         >0: response http status code
 */
int webclient_post(struct webclient_session *session, const char *URI, const char *post_data)
{
    int rc = WEBCLIENT_OK;
    int resp_status = 0;

    RT_ASSERT(session);
    RT_ASSERT(URI);

    rc = webclient_connect(session, URI);
    if (rc != WEBCLIENT_OK)
    {
        /* connect to webclient server failed. */
        return rc;
    }

    rc = webclient_send_header(session, WEBCLIENT_POST);
    if (rc != WEBCLIENT_OK)
    {
        /* send header to webclient server failed. */
        return rc;
    }

    if (post_data)
    {
        webclient_write(session, (unsigned char *) post_data, rt_strlen(post_data));

        /* resolve response data, get http status code */
        resp_status = webclient_handle_response(session);
        if (resp_status != 200)
        {
            LOG_E("post failed, handle response(%d) error.", resp_status);
            return resp_status;
        }
    }

    return resp_status;
}


/**
 * set receive and send data timeout.
 *
 * @param session http session
 * @param millisecond timeout millisecond
 *
 * @return 0: set timeout success
 */
int webclient_set_timeout(struct webclient_session *session, int millisecond)
{
    struct timeval timeout;
    int second = rt_tick_from_millisecond(millisecond) / 1000;

    RT_ASSERT(session);

    timeout.tv_sec = second;
    timeout.tv_usec = 0;

    /* set recv timeout option */
    setsockopt(session->socket, SOL_SOCKET, SO_RCVTIMEO,
               (void *) &timeout, sizeof(timeout));
    setsockopt(session->socket, SOL_SOCKET, SO_SNDTIMEO,
               (void *) &timeout, sizeof(timeout));

    return 0;
}

static int webclient_next_chunk(struct webclient_session *session)
{
    char line[64];
    int length;

    RT_ASSERT(session);

    length = webclient_read_line(session, line, sizeof(line));
    if (length > 0)
    {
        if (rt_strcmp(line, "\r") == 0)
        {
            length = webclient_read_line(session, line, sizeof(line));
            if (length <= 0)
            {
                closesocket(session->socket);
                session->socket = -1;
                return length;
            }
        }
    }
    else
    {
        closesocket(session->socket);
        session->socket = -1;

        return length;
    }

    session->chunk_sz = strtol(line, RT_NULL, 16);
    session->chunk_offset = 0;

    if (session->chunk_sz == 0)
    {
        /* end of chunks */
        closesocket(session->socket);
        session->socket = -1;
    }

    return session->chunk_sz;
}

/**
 *  read data from http server.
 *
 * @param session http session
 * @param buffer read buffer
 * @param length the maximum of read buffer size
 *
 * @return <0: read data error
 *         =0: http server disconnect
 *         >0: successfully read data length
 */
int webclient_read(struct webclient_session *session, unsigned char *buffer, size_t length)
{
    int bytes_read = 0;
    int total_read = 0;
    int left;

    RT_ASSERT(session);

    if (session->socket < 0)
    {
        return -WEBCLIENT_DISCONNECT;
    }

    if (length == 0)
    {
        return 0;
    }

    /* which is transfered as chunk mode */
    if (session->chunk_sz)
    {
        if ((int) length > (session->chunk_sz - session->chunk_offset))
        {
            length = session->chunk_sz - session->chunk_offset;
        }

        bytes_read = webclient_recv(session, buffer, length, 0);
        if (bytes_read <= 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                /* recv timeout */
                return -WEBCLIENT_TIMEOUT;
            }
            else
            {
                closesocket(session->socket);
                session->socket = -1;
                return 0;
            }
        }

        session->chunk_offset += bytes_read;
        if (session->chunk_offset >= session->chunk_sz)
        {
            webclient_next_chunk(session);
        }

        return bytes_read;
    }

    if (session->content_length > 0)
    {
        if (length > session->content_remainder)
        {
            length = session->content_remainder;
        }

        if (length == 0)
        {
            return 0;
        }
    }

    /*
     * Read until: there is an error, we've read "size" bytes or the remote
     * side has closed the connection.
     */
    left = length;
    do
    {
        bytes_read = webclient_recv(session, buffer + total_read, left, 0);
        if (bytes_read <= 0)
        {
#ifdef WEBCLIENT_USING_TLS
            if(session->tls_session && bytes_read == MBEDTLS_ERR_SSL_WANT_READ)
                continue;
#endif  
            LOG_E("receive data error(%d).", bytes_read);

            if (total_read)
            {
                break;
            }
            else
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    /* recv timeout */
                    LOG_E("receive data timeout.");
                    return -WEBCLIENT_TIMEOUT;
                }
                else
                {
                    closesocket(session->socket);
                    session->socket = -1;
                    return 0;
                }
            }
        }

        left -= bytes_read;
        total_read += bytes_read;
    }
    while (left);

    if (session->content_length > 0)
    {
        session->content_remainder -= total_read;
    }

    return total_read;
}

/**
 *  write data to http server.
 *
 * @param session http session
 * @param buffer write buffer
 * @param length write buffer size
 *
 * @return <0: write data error
 *         =0: http server disconnect
 *         >0: successfully write data length
 */
int webclient_write(struct webclient_session *session, const unsigned char *buffer, size_t length)
{
    int bytes_write = 0;
    int total_write = 0;
    int left = length;

    RT_ASSERT(session);

    if (session->socket < 0)
    {
        return -WEBCLIENT_DISCONNECT;
    }

    if (length == 0)
    {
        return 0;
    }

    /* send all of data on the buffer. */
    do
    {
        bytes_write = webclient_send(session, buffer + total_write, left, 0);
        if (bytes_write <= 0)
        {
#ifdef WEBCLIENT_USING_TLS
            if(session->tls_session && bytes_write == MBEDTLS_ERR_SSL_WANT_WRITE)
                continue;
#endif
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                /* send timeout */
                if (total_write)
                {
                    return total_write;
                }
                continue;
                /* TODO: whether return the TIMEOUT
                 * return -WEBCLIENT_TIMEOUT; */
            }
            else
            {
                closesocket(session->socket);
                session->socket = -1;

                if (total_write == 0)
                {
                    return -WEBCLIENT_DISCONNECT;
                }
                break;
            }
        }

        left -= bytes_write;
        total_write += bytes_write;
    }
    while (left);

    return total_write;
}

/**
 * close a webclient client session.
 *
 * @param session http client session
 *
 * @return 0: close success
 */
int webclient_close(struct webclient_session *session)
{
    RT_ASSERT(session);
    
#ifdef WEBCLIENT_USING_TLS
    if (session->tls_session)
    {
        mbedtls_client_close(session->tls_session);
    }
    else
    {
        if (session->socket >= 0)
        {
            closesocket(session->socket); 
            session->socket = -1;
        }
    }
#else
    if (session->socket >= 0)
    {
        closesocket(session->socket);
        session->socket = -1;
    }
#endif

    if (session->host)
    {
        web_free(session->host);
    }

    if (session->req_url)
    {
        web_free(session->req_url);
    }

    if (session->header && session->header->buffer)
    {
        web_free(session->header->buffer);
    }

    if (session->header)
    {
        web_free(session->header);
    }

    if (session)
    {
        web_free(session);
        session = RT_NULL;
    }

    return 0;
}

/**
 * get wenclient request response data.
 *
 * @param session wenclient session
 * @param response response buffer address
 *
 * @return response data size
 */
int webclient_response(struct webclient_session *session, unsigned char **response)
{
    unsigned char *buf_ptr;
    unsigned char *response_buf = 0;
    int length, total_read = 0;

    RT_ASSERT(session);
    RT_ASSERT(response);

    /* initialize response */
    *response = RT_NULL;

    /* not content length field kind */
    if (session->content_length < 0)
    {
        size_t result_sz;

        total_read = 0;
        while (1)
        {
            unsigned char *new_resp = RT_NULL;

            result_sz = total_read + WEBCLIENT_RESPONSE_BUFSZ;
            new_resp = web_realloc(response_buf, result_sz + 1);
            if (new_resp == RT_NULL)
            {
                LOG_E("no memory for realloc new response buffer!");
                break;
            }

            response_buf = new_resp;
            buf_ptr = (unsigned char *) response_buf + total_read;

            /* read result */
            length = webclient_read(session, buf_ptr, result_sz - total_read);
            if (length <= 0)
                break;

            total_read += length;
        }
    }
    else
    {
        int result_sz;

        result_sz = session->content_length;
        response_buf = web_malloc(result_sz + 1);
        if (!response_buf)
        {
            return -WEBCLIENT_NOMEM;
        }

        buf_ptr = (unsigned char *) response_buf;
        for (total_read = 0; total_read < result_sz;)
        {
            length = webclient_read(session, buf_ptr, result_sz - total_read);
            if (length <= 0)
                break;

            buf_ptr += length;
            total_read += length;
        }
    }

    if ((total_read == 0) && (response_buf != 0))
    {
        web_free(response_buf);
        response_buf = RT_NULL;
    }

    if (response_buf)
    {
        *response = response_buf;
        *(response_buf + total_read) = '\0';
    }

    return total_read;
}

/**
 *  send request(GET/POST) to server and get response data.
 *
 * @param URI input server address
 * @param header send header data
 *             = NULL: use default header data, must be GET request
 *            != NULL: user custom header data, GET or POST request
 * @param post_data data sent to the server
 *             = NULL: it is GET request
 *            != NULL: it is POST request
 * @param response response buffer address
 *
 * @return <0: request failed
 *        >=0: response buffer size
 */
int webclient_request(const char *URI, const char *header, const char *post_data, unsigned char **response)
{
    struct webclient_session *session;
    int rc = WEBCLIENT_OK;
    int totle_length;

    RT_ASSERT(URI);

    if (post_data == RT_NULL && response == RT_NULL)
    {
        LOG_E("request get failed, get response data cannot be empty.");
        return -WEBCLIENT_ERROR;
    }

    if (post_data == RT_NULL)
    {
        session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
        if (session == RT_NULL)
        {
            rc = -WEBCLIENT_NOMEM;
            goto __exit;
        }

        if (header != RT_NULL)
        {
            rt_strncpy(session->header->buffer, header, rt_strlen(header));
        }

        if (webclient_get(session, URI) != 200)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }

        totle_length = webclient_response(session, response);
        if (totle_length <= 0)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
    }
    else
    {
        session = webclient_session_create(WEBCLIENT_HEADER_BUFSZ);
        if (session == RT_NULL)
        {
            rc = -WEBCLIENT_NOMEM;
            goto __exit;
        }

        if (header != RT_NULL)
        {
            rt_strncpy(session->header->buffer, header, rt_strlen(header));
        }
        else
        {
            /* build header for upload */
            webclient_header_fields_add(session, "Content-Length: %d\r\n", rt_strlen(post_data));
            webclient_header_fields_add(session, "Content-Type: application/octet-stream\r\n");
        }

        if (webclient_post(session, URI, post_data) != 200)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
        
        totle_length = webclient_response(session, response);
        if (totle_length <= 0)
        {
            rc = -WEBCLIENT_ERROR;
            goto __exit;
        }
    }

    if (header != RT_NULL)
    {
        rt_strncpy(session->header->buffer, header, rt_strlen(header));
    }

__exit:
    if (session)
    {
        webclient_close(session);
        session = RT_NULL;
    }

    if (rc < 0)
    {
        return rc;
    }

    return totle_length;
}
