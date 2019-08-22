/*
 * File      : rt_cld_web.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-06     chenyong     the first version
 */

#ifndef _RT_CLD_WEN_H_
#define _RT_CLD_WEN_H_

#ifdef CLD_USING_SHELL
void cld_web_shell_start(const char * host, int port, const char *log_key);
#endif

#ifdef CLD_USING_LOG
void cld_web_log_start(const char * host, int port, const char *log_key);
#endif

#endif /* _RT_CLD_WEN_H_ */


