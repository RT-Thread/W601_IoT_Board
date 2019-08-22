/*
 * File      : rt_cld_ota.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-06     chenyong     the first version
 */

#ifndef _RT_CLD_OTA_H_
#define _RT_CLD_OTA_H_

/* Download status */
enum CLD_DL_STATUS

{
    DOWNLOAD_OK,
    DOWNLOAD_ERR,
    DOWNLOAD_NOMEM,
    DOWNLOAD_OPEN_FAILED,       /* OTA open url failed */
    DOWNLOAD_RECV_FAILED,       /* OTA recv data failed, network connection failed or network signal is unstable. */
    DOWNLOAD_PART_FAILED,       /* Get partition information failed or perform partition-related operation failed */
};

int rt_cld_ota_init(void);

#endif/* _RT_CLD_OTA_H_ */




