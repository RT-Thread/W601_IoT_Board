# 硬件加解密例程

本例程将演示硬件 AES-CBC 加密及解密功能，以及使用硬件 MD5 和 SHA1 散列算法生成信息摘要。

## 简介

随机物联网大力发展，越来越多的设备接入网络，连接上云端。在使用网络通过的过程中，重要的原始明文被不法分子窃取到之后，将会产生不可估量的后果。此时信息安全显得格外重要。信息的安全交互往往伴随着各种加密解密。本例程将介绍如何使用常见的加解密算法及散列算法。

## 硬件说明

本硬件加解密例程需要使用串口输出功能，无其他依赖。

## 软件说明

硬件加解密的源代码位于 `/examples/27_iot_hw_crypto/applications/main.c` 中。

在 main 函数中，先使用 AES-CBC 将数据进行加密，然后对解密后的数据进行解密。加密完成后，使用 MD5 和 SHA1 两种散列算法生成信息摘要。同时输出一些日志信息。

```c
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
```

硬件加解密具体实现代码在 main() 函数下方。硬件加解密使用流程大致分为 4 个步骤。第一步创建具体加解密类型的上下文。第二步对上下文进行配置，如设置密钥等操作。第三步执行相应的功能，获得处理后的结果。第四步删除上下文，释放资源。

**1. AES-CBC 加解密**

```c
static void hw_aes_cbc(const rt_uint8_t in[32], rt_uint8_t out[32], hwcrypto_mode mode)
{
    struct rt_hwcrypto_ctx *ctx;

    /* 创建一个 AES-CBC 模式的上下文 */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_dufault(), HWCRYPTO_TYPE_AES_CBC);
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
```

**2. HASH 信息摘要**

```c
static void hw_hash(const rt_uint8_t in[32], rt_uint8_t out[32], hwcrypto_type type)
{
    struct rt_hwcrypto_ctx *ctx;

    /* 创建一个 SHA1/MD5 类型的上下文 */
    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_dufault(), type);
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
```

## 运行

### 编译 & 下载

- **MDK**：双击 `project.uvprojx` 打开 MDK5 工程，执行编译。

编译完成后，将固件下载至开发板。

### 运行效果

在 PC 端使用终端工具打开开发板的 `uart0` 串口，设置 115200 8 1 N 。正常运行后，终端输出信息如下：

```shell
 \ | /
- RT -     Thread Operating System
 / | \     4.0.1 build Jun  3 2019
 2006 - 2019 Copyright by rt-thread team
D/HEX Data   : 0000-0008: 00 01 02 03 04 05 06 07    ........
               0008-0010: 08 09 0A 0B 0C 0D 0E 0F    ........
               0010-0018: 10 11 12 13 14 15 16 17    ........
               0018-0020: 18 19 1A 1B 1C 1D 1E 1F    ........
D/HEX AES-enc: 0000-0008: 0A 94 0B B5 41 6E F0 45    ....An.E
               0008-0010: F1 C3 94 58 C6 53 EA 5A    ...X.S.Z
               0010-0018: 3C F4 56 B4 CA 48 8A A3    <.V..H..
               0018-0020: 83 C7 9C 98 B3 47 97 CB    .....G..
D/HEX AES-dec: 0000-0008: 00 01 02 03 04 05 06 07    ........
               0008-0010: 08 09 0A 0B 0C 0D 0E 0F    ........
               0010-0018: 10 11 12 13 14 15 16 17    ........
               0018-0020: 18 19 1A 1B 1C 1D 1E 1F    ........
D/HEX MD5    : 0000-0008: B4 FF CB 23 73 7C EC 31    ...#s|.1
               0008-0010: 5A 4A 4D 1A A2 A6 20 CE    ZJM... .
D/HEX SHA1   : 0000-0008: AE 5B D8 EF EA 53 22 C4    .[...S".
               0008-0010: D9 98 6D 06 68 0A 78 13    ..m.h.x.
               0010-0018: 92 F9 A6 42                ...B
```

## 注意事项

- AES 加解密是以块为单位进行计算的。一个块大小为 16 字节。加解密输入的数据大小需要是 16 字节的整数倍。

- 本例程为硬件加解密，需要运行在支持硬件 AES-CBC 、 MD5 、 SHA1 的芯片上，并且完成了驱动的对接，注册了硬件加解密设备驱动。

## 引用参考

- 《RT-Thread 编程指南》: docs/RT-Thread 编程指南.pdf
