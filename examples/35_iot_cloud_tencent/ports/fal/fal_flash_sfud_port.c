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
#include <sfud.h>
#include <spi_flash_sfud.h>
#include "drv_flash.h"

#ifdef BSP_USING_FLASH

static sfud_flash_t sfud_norflash0;

static int read(long offset, uint8_t *buf, size_t size)
{
    sfud_read(sfud_norflash0, nor_flash0.addr + offset, size, buf);

    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    if (sfud_write(sfud_norflash0, nor_flash0.addr + offset, size, buf) != SFUD_SUCCESS)
    {
        return -1;
    }

    return size;
}

static int erase(long offset, size_t size)
{
    if (sfud_erase(sfud_norflash0, nor_flash0.addr + offset, size) != SFUD_SUCCESS)
    {
        return -1;
    }

    return size;
}

int fal_sfud_init(struct fal_flash_dev *flash)
{
    sfud_flash_t sfud_flash0 = NULL;
    sfud_flash0 = rt_sfud_flash_find_by_dev_name(SPI_FLASH_TYPE_NAME);
    if (NULL == sfud_flash0)
    {
        return -1;
    }
    flash->len = (16 * 1024 * 1024);
    flash->blk_size = 4096;
    flash->addr = 0;
    flash->ops.read = read;
    flash->ops.write = write;
    flash->ops.erase = erase;

    sfud_norflash0 = sfud_flash0;

    return 0;
}

#endif
