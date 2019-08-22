/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-04     tyx          first implementation
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifndef RT_USING_ULOG
#error "please enable ulog component"
#endif

static void hw_aes_cbc(const rt_uint8_t in[32], rt_uint8_t out[32], hwcrypto_mode mode);
static void hw_hash(const rt_uint8_t in[32], rt_uint8_t out[32], hwcrypto_type type);

/* 加密密钥 */
static const rt_uint8_t key[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

int main(void)
{
    rt_uint8_t buf_in[32];
    rt_uint8_t buf_out[32];
    int i;

    /* 填充测试数据 */
    for (i = 0; i < sizeof(buf_in); i++)
    {
        buf_in[i] = (rt_uint8_t)i;
    }
    /* 打印填充的数据 */
    LOG_HEX("Data   ", 8, buf_in, sizeof(buf_in));

    memset(buf_out, 0, sizeof(buf_out));
    /* 对测试数据进行加密 */
    hw_aes_cbc(buf_in, buf_out, HWCRYPTO_MODE_ENCRYPT);

    /* 打印加密后的数据 */
    LOG_HEX("AES-enc", 8, buf_out, sizeof(buf_out));

    memset(buf_in, 0, sizeof(buf_in));
    /* 对加密数据进行解密 */
    hw_aes_cbc(buf_out, buf_in, HWCRYPTO_MODE_DECRYPT);

    /* 打印加密后的数据 */
    LOG_HEX("AES-dec", 8, buf_in, sizeof(buf_in));

    memset(buf_out, 0, sizeof(buf_out));
    /* 对测试数据进行 MD5 运算 */
    hw_hash(buf_in, buf_out, HWCRYPTO_TYPE_MD5);

    /* 打印 16 字节长度的 MD5 结果 */
    LOG_HEX("MD5    ", 8, buf_out, 16);

    memset(buf_out, 0, sizeof(buf_out));
    /* 对测试数据进行 SHA1 运算 */
    hw_hash(buf_in, buf_out, HWCRYPTO_TYPE_SHA1);
    
    /* 打印 20 字节长度的 SHA1 结果 */
    LOG_HEX("SHA1   ", 8, buf_out, 20);

    return 0;
}

static void hw_aes_cbc(const rt_uint8_t in[32], rt_uint8_t out[32], hwcrypto_mode mode)
{
    struct rt_hwcrypto_ctx *ctx;

    /* 创建一个 AES-CBC 模式的上下文 */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), HWCRYPTO_TYPE_AES_CBC);
    if (ctx == RT_NULL)
    {
        LOG_E("create AES-CBC context err!");
        return;
    }
    /* 设置 AES-CBC 加密密钥 */
    rt_hwcrypto_symmetric_setkey(ctx, key, 128);
    /* 执行 AES-CBC 加密/解密 */
    rt_hwcrypto_symmetric_crypt(ctx, mode, 32, in, out);
    /* 删除上下文，释放资源 */
    rt_hwcrypto_symmetric_destroy(ctx);
}

static void hw_hash(const rt_uint8_t in[32], rt_uint8_t out[32], hwcrypto_type type)
{
    struct rt_hwcrypto_ctx *ctx;

    /* 创建一个 SHA1/MD5 类型的上下文 */
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), type);
    if (ctx == RT_NULL)
    {
        LOG_E("create hash[%08x] context err!", type);
        return;
    }
    /* 将输入数据进行 hash 运算 */
    rt_hwcrypto_hash_update(ctx, in, 32);
    /* 获得运算结果 */
    rt_hwcrypto_hash_finish(ctx, out, 32);
    /* 删除上下文，释放资源 */
    rt_hwcrypto_hash_destroy(ctx);
}
