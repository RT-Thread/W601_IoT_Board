/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2019-05-11     Ernest Chen    first implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <cJSON.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 构建 json */
static char *make_json()
{
    char *p;
    cJSON *json_root = cJSON_CreateObject(); /* 创建根索引 json 对象 */
    cJSON *json_sub = cJSON_CreateObject();  /* 创建子索引 json 对象 */
    cJSON *json_array = cJSON_CreateArray(); /* 创建数组 json 对象 */

    /* 判断创建 cjson 是否创建成功 */
    if (RT_NULL == json_root || RT_NULL == json_sub || RT_NULL == json_array)
    {
        LOG_E("Fail to creat cjson");
        rt_free(json_root);
        rt_free(json_sub);
        rt_free(json_array);
        return RT_NULL;
    }
    /* 添加键值对 */
    cJSON_AddStringToObject(json_root, "str", "RT-Thread"); /* 添加字符串型键值对 */
    cJSON_AddNumberToObject(json_root, "num", 123);         /* 添加整型键值对 */

    /* 添加子 json 对象到子 json 对象中 */
    cJSON_AddItemToObject(json_root, "obj", json_sub);
    /* 添加 bool 类型给子 json 对象 */
    cJSON_AddTrueToObject(json_sub, "bool"); /* 添加 bool 型键值对 */

    /* 添加数组 json 对象到根 json 对象中 */
    cJSON_AddItemToObject(json_sub, "array", json_array);
    cJSON_AddStringToObject(json_array, 0, "V3.1.2"); /* 添加数组成员 */
    cJSON_AddStringToObject(json_array, 0, "V4.0.0");

    /* 生成无格式 json 字符串 */
    p = cJSON_PrintUnformatted(json_root);

    cJSON_Delete(json_root);
    return p;
}

/* 解析并且打印 json 的字符串*/
static void parse_string_kv(cJSON *root, char *str)
{
    cJSON *parse;

    parse = cJSON_GetObjectItem(root, str);
    if (RT_NULL == parse)
    {
        LOG_E("Fail to parse json about %s", str);
    }
    else
    {
        LOG_D("%s : %s", str, parse->valuestring);
    }
}

/* 解析并且打印 json 的数字*/
static void parse_num_kv(cJSON *root, char *str)
{
    cJSON *parse;

    parse = cJSON_GetObjectItem(root, str);
    if (RT_NULL == parse)
    {
        LOG_E("Fail to parse json about %s", str);
    }
    else
    {
        LOG_D("%s : %d", str, parse->valueint);
    }
}

/* 解析并且打印 json 的 bool 类型 */
static void parse_bool_kv(cJSON *root, char *str)
{
    cJSON *parse;

    parse = cJSON_GetObjectItem(root, str);
    if (RT_NULL == parse)
    {
        LOG_E("Fail to parse json about %s", str);
    }
    else
    {
        LOG_D("%8s : %d", str, parse->valueint);
    }
}

/* 解析并且打印 json 的键值 */
static cJSON *parse_key(cJSON *root, char *str)
{
    cJSON *parse;

    parse = cJSON_GetObjectItem(root, str);
    if (RT_NULL == parse)
    {
        LOG_E("Fail to parse json about %s", str);
        return RT_NULL;
    }
    if (rt_strcmp("array", str) == 0)
    {
        LOG_D("%9s:", parse->string);
    }
    else
    {
        LOG_D("%s :", parse->string);
    }
    return parse;
}

/* 解析并且打印 json 的数组 */
static cJSON *parse_array(cJSON *root, char *str)
{
    cJSON *parse, *version;
    uint32_t i, array_size;

    version = parse_key(root, "array");
    array_size = cJSON_GetArraySize(version);
    for (i = 0; i < array_size; i++)
    {
        cJSON *item;

        item = cJSON_GetArrayItem(version, i);
        if (RT_NULL == item)
        {
            LOG_E("Fail to parse array json");
            return RT_NULL;
        }
        else
        {
            LOG_D("           %s", item->valuestring);
        }
    }
    return parse;
}

/* 解析构建的json */
static void parse_json(char *p)
{
    cJSON *root, *subordinate;
    RT_ASSERT(p);

    root = cJSON_Parse(p);
    if (RT_NULL == root)
    {
        LOG_E("Fail to parse root json");
        return;
    }

    /* 解析 json 中 CJSON_STR 键对应的字符串 */
    parse_string_kv(root, "str");
    /* 解析 json 中 "num" 键对应的数字 */
    parse_num_kv(root, "num");

    /* 解析子 json */
    subordinate = parse_key(root, "obj");
    /* 解析子 json 中 “bool” 键对应的 bool 值*/
    parse_bool_kv(subordinate, "bool");

    /* 解析 json 数组 */
    parse_array(subordinate, "array");

    cJSON_Delete(root);
}

int main()
{
    /* 构建 json */
    char *p = make_json();

    if (RT_NULL == p)
    {
        return 0;
    }
    LOG_D("make json ->");
    LOG_D("%s", p);

    /* 解析组成的json */
    LOG_D("parse json ->");
    parse_json(p);

    rt_free(p);
    return 0;
}
