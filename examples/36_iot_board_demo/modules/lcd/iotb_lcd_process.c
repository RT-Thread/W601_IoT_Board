/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-17     MurphyZhao   first implementation
 */

#include <rtthread.h>
#include <stdint.h>
#include <stdio.h>
#include <rtdevice.h>
#include <board.h>

#include "iotb_lcd_process.h"
#include "iotb_event.h"

#include <drv_lcd.h>
#include "iotb_lcd_img.h"
#include "iotb_sensor.h"

#include "smartconfig.h"
#include "iotb_workqeue.h"
#include <wlan_mgnt.h>

#include <rthw.h>

#undef DBG_TAG
#undef DBG_LVL
#undef DBG_COLOR
#undef DBG_ENABLE

// #define IOTB_LCD_DEBUG

#define DBG_ENABLE
#define DBG_TAG               "IOTB_LCD"
#ifdef IOTB_LCD_DEBUG
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_LOG /*DBG_INFO DBG_ERROR */
#endif
#define DBG_COLOR
#include <rtdbg.h>

#define IOTB_LCD_THREAD_STACK_SIZE    2048
#define IOTB_ENTER_PM_WAIT_TICK       20

/* Airkiss scan http://story.rt-thread.com/api/getAirkiss */
#define IOTB_SMARTCONFIG_API  "http://story.rt-thread.com/api/getAirkiss"
#define IOTB_RT_CLOUD_BINDING_API "http://iot.rt-thread.com/m/#/login?sn="

static rt_mutex_t lcd_mutex = RT_NULL;
static struct rt_event lcd_event;

static volatile uint8_t iotb_lcd_menu_index = 1;
static volatile uint8_t iotb_lcd_event_enter = 0;
static volatile uint8_t iotb_lcd_event_exit = 0;
static volatile uint8_t iotb_lcd_status_busy = 0;

static void iotb_lcd_show_index_page(iotb_lcd_menu_t *lcd_menu);    /* menu1  */
static void iotb_lcd_show_sensor(iotb_lcd_menu_t *lcd_menu);        /* menu2  */
//static void iotb_lcd_show_axis(iotb_lcd_menu_t *lcd_menu);          /* menu3  */
static void iotb_lcd_show_beep_motor_rgb(iotb_lcd_menu_t *lcd_menu);/* menu4  */
static void iotb_lcd_show_sdcard(iotb_lcd_menu_t *lcd_menu);        /* menu5  */
static void iotb_lcd_show_infrared(iotb_lcd_menu_t *lcd_menu);      /* menu6  */
//static void iotb_lcd_show_music(iotb_lcd_menu_t *lcd_menu);         /* menu7  */
static void iotb_lcd_show_wifiscan(iotb_lcd_menu_t *lcd_menu);      /* menu8  */
static void iotb_lcd_show_wechatscan(iotb_lcd_menu_t *lcd_menu);    /* menu9  */
static void iotb_lcd_show_wificonfig(iotb_lcd_menu_t *lcd_menu);    /* menu10 */
static void iotb_lcd_show_network(iotb_lcd_menu_t *lcd_menu);       /* menu11 */
static void iotb_lcd_show_rt_cloud(iotb_lcd_menu_t *lcd_menu);      /* menu12 */
// static void iotb_lcd_show_lowpower(iotb_lcd_menu_t *lcd_menu);      /* menu13 */
// static void iotb_lcd_show_in_lowpower(iotb_lcd_menu_t *lcd_menu);   /* menu14 */

static const uint16_t menu_refresh_time[IOTB_LCD_MENU_MAX + 1] =
{
    1000, /* menu0  show startup page */
    1000, /* menu1  show HW/FW version */
    5,    /* menu2  show temp/humi/als/ps sensor data */
    2,    /* menu4  show beep/motor/rgb control content */
    100,  /* menu5  show SD card content */
    10,   /* menu6  show infrared value */
//    5,    /* menu7  show music, play wave file */
    100,  /* menu8  show wifi scan result list */
    20,   /* menu9  show wechat scan page */
    20,   /* menu10  show wait connect page */
    2,    /* menu11  show net info content */
    100,   /* menu12 RTT Cloud */
    // 20,   /* menu13 show pm page */
    // 99    /* menu14 enter pm mode*/
};

static volatile iotb_lcd_menu_t lcd_instance[IOTB_LCD_MENU_MAX + 1];
static const iotb_lcd_handle iotb_lcd_handle_array[IOTB_LCD_MENU_MAX + 1] =
{
    RT_NULL,
    (iotb_lcd_handle)iotb_lcd_show_index_page,
    (iotb_lcd_handle)iotb_lcd_show_sensor,
    (iotb_lcd_handle)iotb_lcd_show_beep_motor_rgb,
    (iotb_lcd_handle)iotb_lcd_show_sdcard,
    (iotb_lcd_handle)iotb_lcd_show_infrared,
//    (iotb_lcd_handle)iotb_lcd_show_music,
    (iotb_lcd_handle)iotb_lcd_show_wifiscan,
    (iotb_lcd_handle)iotb_lcd_show_wechatscan,
    (iotb_lcd_handle)iotb_lcd_show_wificonfig,
    (iotb_lcd_handle)iotb_lcd_show_network,
    (iotb_lcd_handle)iotb_lcd_show_rt_cloud,
//    (iotb_lcd_handle)iotb_lcd_show_lowpower,
//    (iotb_lcd_handle)iotb_lcd_show_in_lowpower
};

enum lcd_menu
{
    IOTB_LCD_SHOW_INDEX_PAGE = 1,
    IOTB_LCD_SHOW_SENSOR,
    IOTB_LCD_SHOW_BEEP_MOTOR_RGB,
    IOTB_LCD_SHOW_SDCARD,
    IOTB_LCD_SHOW_INFRARED,
//  IOTB_LCD_SHOW_MUSIC,
    IOTB_LCD_SHOW_WIFISCAN,
    IOTB_LCD_SHOW_WECHATSCAN,
    IOTB_LCD_SHOW_WIFICONFIG,
    IOTB_LCD_SHOW_NETWORK,
    IOTB_LCD_SHOW_RT_CLOUD,
//  IOTB_LCD_SHOW_LOWPOWER,
//  IOTB_LCD_SHOW_IN_LOWPOWER,
};


/**
 * Function declaration
*/
static void iotb_lcd_show(void *arg);

void iotb_lcd_event_put(iotb_lcd_event_t event)
{
    rt_event_send(&lcd_event, (rt_uint32_t)event);
}

/** iotb_lcd_event_get
 * @param event_t: lcd event type
 * @param   event: the event value getting from system
 * @param     clr: 0 - do not clear event flag; 1 - clear event flag
 * @param timeout: set operate time
 * @return RT_EOK - success; others - failed
*/
rt_err_t iotb_lcd_event_get(uint32_t set,
                            uint32_t *event,
                            uint8_t clr,
                            uint32_t timeout)
{
    rt_err_t rst = RT_EOK;
    if (clr)
    {
        rst = rt_event_recv(&lcd_event,
                            set,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            timeout, (rt_uint32_t *)event);
    }
    else
    {
        rst = rt_event_recv(&lcd_event,
                            set,
                            RT_EVENT_FLAG_OR,
                            timeout, (rt_uint32_t *)event);
    }

    if (rst == RT_EOK)
    {
        LOG_D("recv event 0x%x", *event);
    }
    return rst;
}

static uint16_t iotb_lcd_thr_cycle = IOTB_LCD_THR_CYCLE;
void iotb_lcd_thr_set_cycle(uint16_t time)
{
    rt_base_t level = rt_hw_interrupt_disable();
    iotb_lcd_thr_cycle = time;
    rt_hw_interrupt_enable(level);
}

static void iotb_lcd_show(void *arg) /* dynamic thread */
{
    uint8_t menu_index = 1;
    uint16_t cnt = 0;
    uint32_t event = 0;

    while (1)
    {
        LOG_I("When power-on show LCD menu1");

        lcd_instance[menu_index].content_type = IOTB_LCD_STATIC_CONTENT;
        lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_ENTER;
        lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);

        lcd_instance[menu_index].content_type = IOTB_LCD_DYNAMIC_CONTENT;
        lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_NONE;
        lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);

        while (1)
        {
            if (iotb_lcd_event_get(0xffffffff &
                                   (~(IOTB_LCD_EVENT_STOP_DEV_INFO_GET | IOTB_LCD_EVENT_START_DEV_INFO_GET)),
                                   &event, 1, 0) == RT_EOK)
            {
                LOG_D("lcd get event (0x%x); all :%08x", event, 0xffffffff &
                      (~(IOTB_LCD_EVENT_STOP_DEV_INFO_GET | IOTB_LCD_EVENT_START_DEV_INFO_GET)));
                if (event & IOTB_LCD_EVENT_NEXT)
                {
                    menu_index = iotb_lcd_get_menu_index();
                    lcd_instance[menu_index].content_type = IOTB_LCD_CONTENT_NONE;
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_EXIT;
                    /* exit current menu, then enter next menu */
                    lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);

                    menu_index ++;
                    if (menu_index == IOTB_LCD_SHOW_WECHATSCAN) /* WeChat Scan */
                    {
                        if (iotb_sensor_wifi_status_get() == IOTB_WIFI_UP)
                        {
                            menu_index = IOTB_LCD_SHOW_NETWORK; /* skip airkiss config 11*/
                        }
                    }
                    else if (menu_index == (IOTB_LCD_SHOW_WECHATSCAN + 1) || menu_index == (IOTB_LCD_SHOW_WECHATSCAN + 2))
                    {
                        if (iotb_sensor_wifi_status_get() != IOTB_WIFI_UP)
                            menu_index = 13; /* skip `wifi config page and Network page` */
                        else
                            menu_index = IOTB_LCD_SHOW_NETWORK; /* skip `wifi config page` 11*/
                    }
                    else if (menu_index == 11)
                    {
                        menu_index = 1; /* skip `IN PM PAGE` */
                    }

                    menu_index = menu_index > IOTB_LCD_MENU_MAX ? 1 : menu_index;
                    iotb_lcd_update_menu_index(menu_index);
                    // LOG_I("Will show menu %d", menu_index);

                    lcd_instance[menu_index].content_type = IOTB_LCD_STATIC_CONTENT;
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_ENTER;
                    // LOG_I("show menu %d ENTER", menu_index);
                    lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);
                    // LOG_I("show menu %d ENTER FINISH", menu_index);
                    lcd_instance[menu_index].content_type = IOTB_LCD_DYNAMIC_CONTENT;
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_NONE;
                    LOG_I("exit response lcd next event");
                    
                    cnt = lcd_instance[menu_index].refresh_time;
                    // break;
                }
                else if (event & IOTB_LCD_EVENT_EXIT)
                {
                    lcd_instance[menu_index].content_type = IOTB_LCD_CONTENT_NONE;
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_EXIT;
                    lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);
                    /* set current menu event as NONE, and set show NONE */
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_NONE;
                    cnt = 0;
                }
                else if ((event & IOTB_LCD_EVENT_SMARTCONFIG_START) ||
                         (event & IOTB_LCD_EVENT_SMARTCONFIG_STARTED) ||
                         (event & IOTB_LCD_EVENT_SMARTCONFIG_FINISH))
                {
                    menu_index = iotb_lcd_get_menu_index();
                    if (menu_index != IOTB_LCD_SHOW_WECHATSCAN)
                    {
                        /* will enter wechat scan page, first exit current page */
                        lcd_instance[menu_index].content_type = IOTB_LCD_CONTENT_NONE;
                        lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_EXIT;
                        /* exit current menu, then enter next menu */
                        lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);
                    }

                    if (event & IOTB_LCD_EVENT_SMARTCONFIG_START)
                    {
                        menu_index = IOTB_LCD_SHOW_WECHATSCAN;
                    }
                    else if (event & IOTB_LCD_EVENT_SMARTCONFIG_STARTED)
                    {
                        menu_index = IOTB_LCD_SHOW_WIFICONFIG;
                    }
                    else if (event & IOTB_LCD_EVENT_SMARTCONFIG_FINISH)
                    {
                        menu_index = IOTB_LCD_SHOW_NETWORK;
                    }
                    menu_index = menu_index > IOTB_LCD_MENU_MAX ? 1 : menu_index;
                    iotb_lcd_update_menu_index(menu_index);

                    lcd_instance[menu_index].content_type = IOTB_LCD_STATIC_CONTENT;
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_ENTER;
                    lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);
                    lcd_instance[menu_index].content_type = IOTB_LCD_DYNAMIC_CONTENT;
                    lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_NONE;
                    cnt = lcd_instance[menu_index].refresh_time;
                }
                else if (/*(event & IOTB_LCD_EVENT_KEY2) || */
                    (event & IOTB_LCD_EVENT_KEY1) ||
                    (event & IOTB_LCD_EVENT_WKUP))
                {
                    LOG_D("KEY1/WKUP EVENT");
                    if ((menu_index == IOTB_LCD_SHOW_BEEP_MOTOR_RGB) || (menu_index == 7))
                    {
                        lcd_instance[menu_index].content_type = IOTB_LCD_DYNAMIC_CONTENT;
                        /* if (event & IOTB_LCD_EVENT_KEY2)
                         {
                             lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_KEY2;
                         }
                         else */if (event & IOTB_LCD_EVENT_KEY1)
                        {
                            lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_KEY1;
                        }
                        else if (event & IOTB_LCD_EVENT_WKUP)
                        {
                            lcd_instance[menu_index].current_event = IOTB_LCD_EVENT_WKUP;
                        }
                        lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);
                    }
                }
            }

            /* TODO Need to wait until the start event runs, First check if is sho NONE, if yes skip */
            if ((lcd_instance[menu_index].content_type == IOTB_LCD_DYNAMIC_CONTENT) &&
                    (cnt >= lcd_instance[menu_index].refresh_time))
            {
                cnt = 0;
                lcd_instance[menu_index].lcd_handle((void *)&lcd_instance[menu_index]);
            }

            rt_thread_mdelay(iotb_lcd_thr_cycle);
            cnt ++;
        }
    }
}

#include <board.h>
#include <iotb_version.h>

void iotb_lcd_show_startup_page(void)
{
    /* show RT-Thread logo */
    lcd_show_image(0, 43, 240, 69, image_rttlogo);
    lcd_show_image(0, 123, 240, 74, gImage_yuanzilogo240x74);
}

static void iotb_lcd_show_index_page(iotb_lcd_menu_t *lcd_menu)
{
    LOG_D("show [index page]");
    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        char buf[21];

        /* clear lcd */
        lcd_clear(WHITE);

        /* set the background color and foreground color */
        lcd_set_color(WHITE, BLACK);

        lcd_show_string(48, 8, 32, "IoT Board");

        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), "RT-Thread: %d.%d.%d", RT_VERSION, RT_SUBVERSION, RT_REVISION);
        lcd_show_string(24, 59 - 2, 24, buf);

        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), "HardWare : %x.%x", (uint8_t)(HARDWARE_VERSION >> 8), (uint8_t)HARDWARE_VERSION);
        lcd_show_string(24, 59 - 2 + 28, 24,  buf); //HARD_VERSION 0x0201U

        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), "Firmware : %x.%x", (uint8_t)(IOTBOARD_APP_VERSION >> 8), (uint8_t)IOTBOARD_APP_VERSION);
        lcd_show_string(24, 59 - 2 + 28 + 28, 24,  buf); //IOTBOARD_APP_VERSION

        lcd_show_string(0, 78 + 28 + 28 + 28, 32, "KEY0   NextPage");
        lcd_show_image(76, 78 + 28 + 28 + 28 + 4, 24, 24, gImage_figner24x24);
        lcd_set_color(WHITE, RED);
        lcd_show_string(0, 78 + 28 + 28 + 28, 32, "KEY0");
    }
}

static void iotb_lcd_show_sensor(iotb_lcd_menu_t *lcd_menu)
{
    char buf[48];
    iotb_sensor_data_t sensor_data;

    iotb_sensor_data_upload(iotb_sensor_data_result_get(), &sensor_data);

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [sensor page]");

        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(72, 8, 32, "sensor");

        lcd_show_image(16, 38 + 24, 24, 24, gImage_tem24x24);
        if (sensor_data.aht10_temp_status != RT_EOK)
        {
            lcd_show_string(42, 38 + 24, 24, "temp   -----C");
        }

        lcd_show_image(16, 38 + 24 + 20 + 24, 24, 24, gImage_humi24x24);
        if (sensor_data.aht10_humi_status != RT_EOK)
        {
            lcd_show_string(42, 38 + 24 + 20 + 24, 24, "humi   -----%");
        }

        lcd_show_image(16, 38 + 24 + 20 + 24 + 20 + 24, 24, 24, gImage_sun24x24);
        if (sensor_data.ap3216_als_status != RT_EOK)
        {
            lcd_show_string(42, 38 + 24 + 20 + 24 + 20 + 24, 24, "light  -----lux");
        }

        lcd_show_image(16, 38 + 24 + 20 + 24 + 20 + 24 + 20 + 24, 24, 24, gImage_dis24x24);
        if (sensor_data.ap3216_ps_status != RT_EOK)
        {
            lcd_show_string(42, 38 + 24 + 20 + 24 + 20 + 24 + 20 + 24, 24, "ps     -----ps");
        }

        lcd_set_color(WHITE, GRAY240);
        lcd_draw_line(16 + 24, 38 + 24 + 28, 240 - 40, 38 + 24 + 28);
        lcd_draw_line(16 + 24, 38 + 24 + 20 + 24 + 28, 240 - 40, 38 + 24 + 20 + 24 + 28);
        lcd_draw_line(16 + 24, 38 + 24 + 20 + 24 + 20 + 24 + 28, 240 - 40, 38 + 24 + 20 + 24 + 20 + 24 + 28);
        lcd_draw_line(16 + 24, 38 + 24 + 20 + 24 + 20 + 24 + 20 + 24 + 28,
                      240 - 40, 38 + 24 + 20 + 24 + 20 + 24 + 20 + 24 + 28);
    }

    lcd_set_color(WHITE, BLACK);

    if (sensor_data.aht10_temp_status == RT_EOK)
    {
        rt_memset(buf, 0x0, sizeof(buf));
        snprintf(buf, sizeof(buf), "temp   %5.1fC",
                 sensor_data.aht10_data_temp);
        LOG_D(buf);
        lcd_show_string(42, 38 + 24, 24, buf);
    }

    if (sensor_data.aht10_humi_status == RT_EOK)
    {
        rt_memset(buf, 0x0, sizeof(buf));
        snprintf(buf, sizeof(buf), "humi   %5.1f%%",
                 sensor_data.aht10_data_humi);
        LOG_D(buf);
        lcd_show_string(42, 38 + 24 + 20 + 24, 24, buf);
    }

    if (sensor_data.ap3216_als_status == RT_EOK)
    {
        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), "light %6dlux",
                    (uint16_t)sensor_data.ap3216_data_als);
        lcd_show_string(42, 38 + 24 + 20 + 24 + 20 + 24, 24, buf);
    }

    if (sensor_data.ap3216_ps_status == RT_EOK)
    {
        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), "ps    %6dps",
                    sensor_data.ap3216_data_ps);
        lcd_show_string(42, 38 + 24 + 20 + 24 + 20 + 24 + 20 + 24, 24, buf);
    }
}

static rt_bool_t iotb_lcd_motor_status = 0;
static rt_bool_t iotb_lcd_beep_status = 0;
static uint8_t iotb_lcd_rgb_status = 3;

static void iotb_lcd_show_beep_motor_rgb(iotb_lcd_menu_t *lcd_menu)
{
    if (lcd_menu->current_event == IOTB_LCD_EVENT_EXIT) // exit
    {
        iotb_sensor_beep_ctl(IOTB_SENSOR_BEEP_DEINIT);
        iotb_sensor_rgb_ctl(IOTB_SENSOR_RGB_DEINIT);
        iotb_lcd_rgb_status = 3;
        return;
    }
    else if (lcd_menu->current_event == IOTB_LCD_EVENT_ENTER)  // enter
    {
        iotb_sensor_beep_ctl(IOTB_SENSOR_BEEP_INIT);
        iotb_sensor_rgb_ctl(IOTB_SENSOR_RGB_INIT);
        iotb_lcd_rgb_status = 3;
    }

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [beep/rgb page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(8,  8, 32,                "BEEP/RGB");

        lcd_show_string(8,  38 + 40, 32,           "WK_UP    BEEF");
        lcd_show_string(8,  38 + 40 + 40 + 40, 32, "KEY1     RGB");
        lcd_show_image(100, 38 + 40, 24, 24, gImage_figner24x24);
//        lcd_show_image(100, 38 + 40 + 40, 24, 24, gImage_figner24x24);
        lcd_show_image(100, 38 + 40 + 40 + 40, 24, 24, gImage_figner24x24);
    }
    else if (lcd_menu->content_type == IOTB_LCD_DYNAMIC_CONTENT)
    {
        if (lcd_menu->current_event == IOTB_LCD_EVENT_WKUP)
        {
            lcd_menu->current_event = IOTB_LCD_EVENT_NONE;
            iotb_lcd_beep_status = !iotb_lcd_beep_status;
            if (iotb_lcd_beep_status)
            {
                lcd_show_image(100, 38 + 40, 24, 24, gImage_figner_red24x24);
                iotb_sensor_beep_ctl(IOTB_SENSOR_BEEP_OPEN);
            }
            else
            {
                lcd_show_image(100, 38 + 40, 24, 24, gImage_figner24x24);
                iotb_sensor_beep_ctl(IOTB_SENSOR_BEEP_CLOSE);
            }
        }
        else if (lcd_menu->current_event == IOTB_LCD_EVENT_KEY1)
        {
            lcd_menu->current_event = IOTB_LCD_EVENT_NONE;
            if (iotb_lcd_rgb_status < (uint8_t)IOTB_SENSOR_RGB_CLOSE)
            {
                LOG_I("show RGB (%d)", iotb_lcd_rgb_status);
                iotb_sensor_rgb_ctl((iotb_sensor_rgb_t)(iotb_lcd_rgb_status));
                lcd_show_image(100, 38 + 40 + 40 + 40, 24, 24, gImage_figner_red24x24);
                iotb_lcd_rgb_status ++;
            }
            else
            {
                iotb_lcd_rgb_status = 3;
                lcd_show_image(100, 38 + 40 + 40 + 40, 24, 24, gImage_figner24x24);
                iotb_sensor_rgb_ctl(IOTB_SENSOR_RGB_CLOSE);
            }
        }

    }
}

#include <dfs_fs.h>
#include <dfs_posix.h>

static void iotb_lcd_show_sdcard(iotb_lcd_menu_t *lcd_menu)
{
    struct dirent *d;
    DIR *dirp = RT_NULL;
    uint8_t line = 0;
    char buf[16];

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [sdcard page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(64,  8, 32, "SD Card");

        /* init with in power-on */
        if (iotb_sensor_sdcard_fs_status_get() == 0)
        {
            lcd_show_string(24, 52 + 27 * 2, 24, " Insert SD card");
            lcd_show_string(24, 52 + 27 * 3, 24, " before power-on");
        }
    }

    if (iotb_sensor_sdcard_fs_status_get() == 0)
    {
        return;
    }

    dirp = opendir("/");
    if (dirp == RT_NULL)
    {
        LOG_E("open directory error!");
        return;
    }
    else
    {
        while ((d = readdir(dirp)) != RT_NULL)
        {
            LOG_D("   %s, type:%d", d->d_name, d->d_type);
            lcd_set_color(WHITE, BLACK);

            rt_memset(buf, 0x0, sizeof(buf));
            rt_snprintf(buf, sizeof(buf), "%-16s", d->d_name);
            lcd_show_string(8,  52 + 28 * line, 24, buf);
            if (d->d_type == 1) // file
            {
                lcd_set_color(WHITE, GRAY187);
                lcd_show_string(240 - 48,  52 + 28 * line, 24, "file");
            }
            else if (d->d_type == 2) //dir
            {
                lcd_set_color(WHITE, GRAY187);
                lcd_show_string(240 - 48,  52 + 28 * line, 24, "dir ");
            }

            if (line > 0)
            {
                lcd_set_color(WHITE, GRAY240);
                lcd_draw_line(8, 52 + 28 * line - 2, 240 - 8, 52 + 28 * line - 2);
            }

            line ++;
            if (line > 5)
            {
                break;
            }
        }
        if ((52 + 28 * line + 1) <= 240)
        {
            lcd_set_color(WHITE, GRAY240);
            lcd_draw_line(8, 52 + 28 * line - 2, 240 - 8, 52 + 28 * line - 2);
        }
    }
    closedir(dirp);
    dirp = RT_NULL;
}

/* menu6 */
static void iotb_lcd_show_infrared(iotb_lcd_menu_t *lcd_menu)
{
    char buf[5];
    iotb_sensor_data_t sensor_data;
    iotb_sensor_data_upload(iotb_sensor_data_result_get(), &sensor_data);
    rt_memset(buf, 0x0, sizeof(buf));
    rt_snprintf(buf, sizeof(buf), "0x%02X", sensor_data.infrared_receive);

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [infrared page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(56,  8, 32, "Infrared");
        lcd_show_string(22,  120 - 34, 32, "Decode Value");
    }
    lcd_show_string(88,  120, 32, buf);
}

static void iotb_lcd_show_wifiscan(iotb_lcd_menu_t *lcd_menu)
{
    if (lcd_menu->current_event == IOTB_LCD_EVENT_EXIT) // exit
    {
        LOG_I("Exit wifiscan page!");
        return;
    }

    /* TODO Check if the wifi device exists, if not show `No wifi dev` */
    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [wifi scan page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);

        lcd_show_image(10, 8, 24, 24, gImage_wifi_connect24x24);
        lcd_show_string(48, 8, 32, "WiFi Scan");

        if (!iotb_sensor_wifi_isinited())
        {
            lcd_show_string(0, 52 + 27 * 2, 24, " WiFi Uninitialized ");
            return;
        }
        lcd_show_string(0, 52 + 27 * 2, 24, "  waiting scan ...  ");
    }

    if (iotb_sensor_wifi_isinited())
    {
        char ssid_buf[21];
        uint8_t line = 0;
        int32_t scan_num;

        iotb_sensor_scan_data_t scan_data = iotb_sensor_scan_data_get();

        lcd_fill(0, 52, 240, 240, WHITE);

        scan_num = scan_data.num;
        while ((line < 7) && (scan_num > 0) && (scan_num --))
        {
            rt_memset(ssid_buf, 0x0, sizeof(ssid_buf));
            rt_snprintf(ssid_buf,
                        sizeof(ssid_buf),
                        "%-20s",
                        &(scan_data.info[line].ssid.val[0]));

            lcd_set_color(WHITE, BLACK);
            lcd_show_string(0,  52 + 27 * line, 24, ssid_buf);

            if (line > 0)
            {
                lcd_set_color(WHITE, GRAY240);
                lcd_draw_line(4, 52 + 27 * line - 1, 240 - 4, 52 + 27 * line - 1);
            }

            line ++;
        }

        if ((52 + 27 * line + 1) <= 240)
        {
            lcd_set_color(WHITE, GRAY240);
            lcd_draw_line(4, 52 + 27 * line - 1, 240 - 4, 52 + 27 * line - 1);
        }

        if (scan_data.status == 0)
        {
            iotb_workqueue_dowork(iotb_sensor_wifi_scan, iotb_sensor_scan_handle_get());
        }
    }
}

/* Airkiss scan http://story.rt-thread.com/api/getAirkiss */
static void iotb_lcd_show_wechatscan(iotb_lcd_menu_t *lcd_menu)
{
    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [wechat scan page] scan&config wifi");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(32,  2, 32, "WeChat Scan");

        if (!iotb_sensor_wifi_isinited())
        {
            lcd_show_string(0, 52 + 27 * 2, 24, " WiFi Uninitialized ");
            return;
        }

        /* QR SIZE = (4 * version + 17)*enlargement = 165 */
        lcd_show_qrcode(37, 55, 4,
                        ECC_LOW, IOTB_SMARTCONFIG_API, 5);
    }

    if (lcd_menu->current_event == IOTB_LCD_EVENT_EXIT)
    {
        LOG_D("Receive exit event, stop airkiss!");
        iotb_smartconfig_stop();
        return;
    }
    else if ((lcd_menu->current_event == IOTB_LCD_EVENT_ENTER) &&
             iotb_sensor_wifi_isinited())
    {
        LOG_D("Start airkiss...");
        iotb_smartconfig_start();
    }

    if ((lcd_menu->content_type == IOTB_LCD_DYNAMIC_CONTENT) &&
            iotb_sensor_wifi_isinited())
    {
        if (iotb_smartconfig_is_started())
        {
            iotb_event_msg_t msg;
            msg.event.event_src = IOTB_EVENT_SRC_WIFI;
            msg.event.event_type = IOTB_EVENT_TYPE_WIFI_AIRKISS_STARTED;
            iotb_event_put(&msg);
            iotb_smartconfig_is_started_clr();
            LOG_D("put lock channel event!");
        }
    }
}

static void iotb_lcd_show_wificonfig(iotb_lcd_menu_t *lcd_menu)
{
    if (lcd_menu->current_event == IOTB_LCD_EVENT_EXIT)
    {
        LOG_D("Receive exit event, stop smartconfig!");
        /* TODO If config success, Do you need to stop configuring? */
        iotb_smartconfig_stop();
        return;
    }

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [wifi config page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(32, 8, 32, "WiFi Config");
        lcd_show_string(15, 120, 24, "waiting connect...");
    }
    else if (lcd_menu->content_type == IOTB_LCD_DYNAMIC_CONTENT)
    {
        iotb_smartconfig_t *iotb_menu8_smartconfig_s;

        iotb_menu8_smartconfig_s = iotb_smartconfig_result_get();
        if (iotb_smartconfig_complete_get() &&
                (iotb_menu8_smartconfig_s->iotb_smartconfig_is_finished == 1))
        {
            if (iotb_sensor_wifi_status_get() == IOTB_WIFI_UP)
            {
                iotb_event_msg_t msg;

                lcd_fill(0, 60, 240, 240, WHITE);
                lcd_show_string(28, 120, 24, "Join ap success");

                // rt_wlan_config_autoreconnect(1);
                iotb_smartconfig_is_started_clr();
                iotb_smartconfig_complete_clr();
                rt_thread_mdelay(2000); /*DBG*/

                msg.event.event_src = IOTB_EVENT_SRC_WIFI;
                msg.event.event_type = IOTB_EVENT_TYPE_WIFI_AIRKISS_SUCCESS;
                iotb_event_put(&msg);
            }
            else if (iotb_menu8_smartconfig_s->iotb_smartconfig_join_is_succ == 0)
            {
                char buf[21];

                lcd_fill(0, 60, 240, 240, WHITE);
                lcd_show_string(0, 160 - 28 - 28 - 28, 24, "Smartconfig finish");

                rt_memset(buf, 0x0, sizeof(buf));
                rt_snprintf(buf, sizeof(buf), "ssid %-15s", iotb_menu8_smartconfig_s->ssid);
                lcd_show_string(0, 160 - 28 - 28, 24, buf);

                rt_memset(buf, 0x0, sizeof(buf));
                rt_snprintf(buf, sizeof(buf), "pwd  %-15s", iotb_menu8_smartconfig_s->pwd);
                lcd_show_string(0, 160 - 28, 24, buf);

                lcd_show_string(0, 160, 24, "waiting ip up ...");

                rt_thread_mdelay(2000); /*DBG*/
            }
            else if (iotb_menu8_smartconfig_s->iotb_smartconfig_join_is_succ == 2)
            {
                lcd_fill(0, 60, 240, 240, WHITE);
                lcd_show_string(36, 120, 24, "Join ap failed");
                lcd_show_string(36, 120 + 28, 32, "WK_UP Retry");

                rt_thread_mdelay(2000); /*DBG*/
            }
        }
        else
        {
            lcd_show_string(15, 120, 24, "waiting connect...");
        }
    }
}

typedef struct
{
    char wan_ip_result[24];

    uint16_t status; /* 0: read finish; 1: not finish */
} iotb_lcd_menu9_wan_data_t;

static iotb_lcd_menu9_wan_data_t iotb_lcd_menu9_wan_data =
{
    .wan_ip_result = {0},
    .status = 0 /* 0: read finish; 1: not finish */
};

static void iotb_lcd_menu9_wan(void *arg)
{
    iotb_lcd_menu9_wan_data_t *wan_s = (iotb_lcd_menu9_wan_data_t *)arg;
    if (wan_s == NULL)
    {
        return;
    }

    LOG_D("iotb_lcd_show_network_sensor!");

    wan_s->status = 1; /* 0: read finish; 1: not finish */

    /* execute synchronous scan function */
    iotb_web_api_wan_get(wan_s->wan_ip_result, sizeof(wan_s->wan_ip_result));

    wan_s->status = 0; /* 0: read finish; 1: not finish */
}

static void iotb_lcd_show_network(iotb_lcd_menu_t *lcd_menu)
{
    char buf[21];

    time_t now;
    struct tm *_iotb_tm;
    /* output current time */
    now = time(RT_NULL);
    _iotb_tm = localtime(&now);

    LOG_D("date: %s; %d-%d-%d:%d-%d-%d", ctime(&now),
          (_iotb_tm->tm_year + 1900),
          _iotb_tm->tm_mon + 1,
          _iotb_tm->tm_mday,
          _iotb_tm->tm_hour,
          _iotb_tm->tm_min,
          _iotb_tm->tm_sec);

    if (lcd_menu->current_event == IOTB_LCD_EVENT_EXIT) // exit
    {
        LOG_I("Exit network page!");
        return;
    }

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        struct rt_wlan_info info;
        char wan_ip[16] = "0.0.0.0";

        LOG_D("show [network page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);

        lcd_show_image(10, 8, 24, 24, gImage_wifi_connect24x24);
        lcd_show_string(64,  8, 32, "Network");

        rt_memset(buf, 0x0, sizeof(buf));
        rt_memset(&info, 0, sizeof(struct rt_wlan_info));
        rt_wlan_get_info(&info);
        rt_snprintf(buf, sizeof(buf), " ssid %-.15s", &info.ssid.val[0]);
        lcd_show_string(0, 26 + 34, 24, buf);

        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), " LAN %-.15s", iotb_sensor_wifi_sta_ip_get());
        lcd_show_string(0, 26 + 34 + 28, 24, buf);

        rt_memset(buf, 0x0, sizeof(buf));
        rt_snprintf(buf, sizeof(buf), " WAN %-.15s", wan_ip);
        lcd_show_string(0, 26 + 34 + 28 + 28, 24, buf);

        if (iotb_sensor_wifi_status_get() == IOTB_WIFI_UP)
        {
            rt_memset(iotb_lcd_menu9_wan_data.wan_ip_result, 0x0,
                      sizeof(iotb_lcd_menu9_wan_data.wan_ip_result));
            iotb_workqueue_dowork(iotb_lcd_menu9_wan, &iotb_lcd_menu9_wan_data);
        }

        rt_memset(buf, 0x0, sizeof(buf));
        snprintf(buf, sizeof(buf), " DATE %4d-%02d-%02d",
                 _iotb_tm->tm_year + 1900,
                 _iotb_tm->tm_mon + 1,
                 _iotb_tm->tm_mday);
        lcd_show_string(0, 26 + 34 + 28 + 28 + 28, 24, buf);

        rt_memset(buf, 0x0, sizeof(buf));
        snprintf(buf, sizeof(buf), " TIME %02d:%02d:%02d",
                 _iotb_tm->tm_hour,
                 _iotb_tm->tm_min,
                 _iotb_tm->tm_sec);
        lcd_show_string(0, 26 + 34 + 28 + 28 + 28 + 28, 24, buf);

        lcd_set_color(WHITE, GRAY240);
        lcd_draw_line(4, 26 + 34 + 24, 240 - 4, 26 + 34 + 24);
        lcd_draw_line(4, 26 + 34 + 28 + 24, 240 - 4, 26 + 34 + 28 + 24);
        lcd_draw_line(4, 26 + 34 + 28 + 28 + 24, 240 - 4, 26 + 34 + 28 + 28 + 24);
        lcd_draw_line(4, 26 + 34 + 28 + 28 + 28 + 24, 240 - 4, 26 + 34 + 28 + 28 + 28 + 24);
        lcd_draw_line(4, 26 + 34 + 28 + 28 + 28 + 28 + 24, 240 - 4, 26 + 34 + 28 + 28 + 28 + 28 + 24);
    }
    else if (lcd_menu->content_type == IOTB_LCD_DYNAMIC_CONTENT)
    {
        lcd_set_color(WHITE, BLACK);

        rt_memset(buf, 0x0, sizeof(buf));
        snprintf(buf, sizeof(buf), " DATE %4d-%02d-%02d",
                 _iotb_tm->tm_year + 1900,
                 _iotb_tm->tm_mon + 1,
                 _iotb_tm->tm_mday);
        lcd_show_string(0, 26 + 34 + 28 + 28 + 28, 24, buf);

        rt_memset(buf, 0x0, sizeof(buf));
        snprintf(buf, sizeof(buf), " TIME %02d:%02d:%02d",
                 _iotb_tm->tm_hour,
                 _iotb_tm->tm_min,
                 _iotb_tm->tm_sec);
        lcd_show_string(0, 26 + 34 + 28 + 28 + 28 + 28, 24, buf);

        if (iotb_lcd_menu9_wan_data.status == 0)
        {
            lcd_set_color(WHITE, BLACK);
            if (rt_strlen(iotb_lcd_menu9_wan_data.wan_ip_result))
            {
                rt_memset(buf, 0x0, sizeof(buf));
                rt_snprintf(buf, sizeof(buf), " WAN %-.15s", iotb_lcd_menu9_wan_data.wan_ip_result);
                lcd_show_string(0, 26 + 34 + 28 + 28, 24, buf);
            }
        }
    }
}

/* 10 */
static void iotb_lcd_show_rt_cloud(iotb_lcd_menu_t *lcd_menu)
{
    rt_uint32_t cpuid[3] = {0};
    char buf[256];

    if (lcd_menu->current_event == IOTB_LCD_EVENT_ENTER)
    {
        iotb_lcd_event_put(IOTB_LCD_EVENT_START_DEV_INFO_GET);
    }
    else if (lcd_menu->current_event == IOTB_LCD_EVENT_EXIT)
    {
        LOG_I("exit rtcloud page. stop get dev info");
        iotb_lcd_event_put(IOTB_LCD_EVENT_STOP_DEV_INFO_GET);
        return;
    }

    if (lcd_menu->content_type == IOTB_LCD_STATIC_CONTENT)
    {
        LOG_D("show [rt cloud page]");
        lcd_clear(WHITE);
        lcd_set_color(WHITE, BLACK);
        lcd_show_string(0, 4, 32,  "RT-Thread Cloud");
        if (!iotb_rtcld_active_status_get())
        {
            rt_uint32_t udid[2] = {0};
            lcd_show_string(8, 38, 32, "Scan & Binding");
            if (rt_wlan_get_mac((rt_uint8_t *)udid) != RT_EOK)
            {
                LOG_E("get mac addr err!! exit");
                return;
            }
            memset(buf, 0x0, sizeof(buf));
            rt_snprintf(buf, sizeof(buf), "%s%08x%08x%08x", IOTB_RT_CLOUD_BINDING_API, udid[0], udid[1], udid[1]);
            LOG_I("RT-Cloud binding API: %s", buf);

            /* QR SIZE = (4 * version + 17)*enlargement */
            lcd_show_qrcode(54, 86, 4, ECC_LOW, buf, 4);
        }
        else
        {
            lcd_fill(0, 38, 240, 240, WHITE);
            lcd_show_string(42,  96, 24,  "device online");
            lcd_show_string(72,  96 + 34, 24,  "view at");
            lcd_show_string(18,  96 + 34 + 26, 24,  "iot.rt-thread.com");
            lcd_menu->refresh_time = 65535;
        }
    }
    else if ((lcd_menu->content_type == IOTB_LCD_DYNAMIC_CONTENT) &&
             (iotb_sensor_wifi_status_get() == IOTB_WIFI_UP))
    {
        int flg = 1;
        if (iotb_rtcld_active_status_get() && flg)
        {
            lcd_fill(0, 38, 240, 240, WHITE);
            lcd_show_string(42,  96 - 12, 24,  "device online");
            lcd_show_string(72,  96 + 34, 24,  "view at");
            lcd_show_string(18,  96 + 34 + 26, 24,  "iot.rt-thread.com");
            lcd_menu->refresh_time = 65535;
            flg = 0;
        }
    }
}

void iotb_lcd_update_menu_index(uint8_t menu_index)
{
    rt_mutex_take(lcd_mutex, RT_WAITING_FOREVER);
    iotb_lcd_menu_index = menu_index;
    rt_mutex_release(lcd_mutex);
}

uint8_t iotb_lcd_get_menu_index(void)
{
    uint8_t menu_index;
    rt_mutex_take(lcd_mutex, RT_WAITING_FOREVER);
    menu_index = iotb_lcd_menu_index;
    rt_mutex_release(lcd_mutex);
    return menu_index;
}

static void iotb_lcd_menu_init(void)
{
    rt_memset((void *)lcd_instance, 0x0, sizeof(lcd_instance));

    for (int i = 1; i < IOTB_LCD_MENU_MAX + 1; i++)//iotb_lcd_handle_array
    {
        lcd_instance[i].menu = i;
        lcd_instance[i].refresh_time = menu_refresh_time[i];
        lcd_instance[i].content_type = IOTB_LCD_CONTENT_NONE;
        lcd_instance[i].current_event = IOTB_LCD_EVENT_NONE;
        lcd_instance[i].lcd_handle = iotb_lcd_handle_array[i];
    }
}

void iotb_lcd_start(void)
{
    rt_thread_t iotb_lcd_tid;
    rt_err_t result = RT_EOK;

    iotb_lcd_menu_init();

    lcd_mutex = rt_mutex_create("lcd_mutex", RT_IPC_FLAG_FIFO);
    RT_ASSERT(lcd_mutex != RT_NULL);

    result = rt_event_init(&lcd_event, "lcd_event", RT_IPC_FLAG_FIFO);
    if (result != RT_EOK)
    {
        LOG_E("Creat event error!");
    }

    /* create lcd show thread 'iotb_lcd_show'*/
    iotb_lcd_tid = rt_thread_create("lcd_thr",
                                    iotb_lcd_show,
                                    RT_NULL,
                                    IOTB_LCD_THREAD_STACK_SIZE, RT_THREAD_PRIORITY_MAX / 2 - 4, 50);

    if (iotb_lcd_tid != RT_NULL)
    {
        rt_thread_startup(iotb_lcd_tid);
    }
}
