/*
 * File      : fal_cfg.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-21     MurphyZhao   the first version
 */
#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <board.h>

#define FAL_NOR_FLASH_NAME "norflash"

extern struct fal_flash_dev nor_flash0;
extern struct fal_flash_dev w60x_onchip;

/* flash device table */
#define FAL_FLASH_DEV_TABLE \
{                           \
    &w60x_onchip,           \
    &nor_flash0,            \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                                                            \
{                                                                                                                 \
    {FAL_PART_MAGIC_WROD, "app",        "w60x_onchip",                                         0, 959 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "easyflash",  FAL_NOR_FLASH_NAME,                                   0, 1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "download",   FAL_NOR_FLASH_NAME,                       (1024) * 1024, 1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "font",       FAL_NOR_FLASH_NAME,            (1024 + 1024) * 1024, 7 * 1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "filesystem", FAL_NOR_FLASH_NAME, (1024 + 1024 + 7 * 1024) * 1024, 7 * 1024 * 1024, 0}, \
}
#endif /* _FAL_CFG_H_ */
#endif
