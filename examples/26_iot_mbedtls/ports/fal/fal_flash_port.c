/*
 * File      : fal_flash_sfud_port.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-26     armink       the first version
 * 2018-08-21     MurphyZhao   update to stm32l4xx
 */

#include <fal.h>
#include "drv_flash.h"

static int init(void);
static int read(long offset, uint8_t *buf, size_t size);
static int write(long offset, const uint8_t *buf, size_t size);
static int erase(long offset, size_t size);
extern int fal_sfud_init(struct fal_flash_dev *flash);

static int init(void)
{
    wm_flash_init();
    w60x_onchip.len = wm_flash_total();
    w60x_onchip.blk_size = wm_flash_blksize();
    w60x_onchip.addr = wm_flash_addr();     
    
    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    return wm_flash_read(offset, buf, size);
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    return wm_flash_write(offset, (void *)buf, size);
}

static int erase(long offset, size_t size)
{
    return wm_flash_erase(offset, size);
}

#if defined(SOC_W600_A8xx) || defined(SOC_W601_A8xx)
static int chip_read(long offset, uint8_t *buf, size_t size)
{
    return w60x_onchip.ops.read(offset + 0x100000, buf, size);
}

static int chip_write(long offset, const uint8_t *buf, size_t size)
{
    return w60x_onchip.ops.write(offset + 0x100000, (void *)buf, size);
}

static int chip_erase(long offset, size_t size)
{
    return w60x_onchip.ops.erase(offset + 0x100000, size);
}
#endif

static int nor_init(void)
{
    int res = 0;

#if defined(SOC_W600_A8xx) || defined(SOC_W601_A8xx)
    wm_flash_init();

    if (wm_flash_total() == 0x200000)
    {
        nor_flash0.len = wm_flash_total() - 0x100000;
        nor_flash0.blk_size = wm_flash_blksize();
        nor_flash0.addr = 0;
        nor_flash0.ops.read =  chip_read;
        nor_flash0.ops.write = chip_write;
        nor_flash0.ops.erase = chip_erase;
    }
    else
#endif
    {
#ifdef BSP_USING_FLASH
        res = fal_sfud_init(&nor_flash0);
#else
    res = -1;
#endif
    }

    return res;
}

struct fal_flash_dev w60x_onchip = {"w60x_onchip", 0, 0, 0, {init, read, write, erase}};
struct fal_flash_dev nor_flash0 = { "norflash", 0, 0, 0, {nor_init, read, write, erase}};
