/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-01     Ernest       the first version
 */
#ifndef __PIN_CONFIG_H__
#define __PIN_CONFIG_H__

// UART
#define WM_UART2_RX_PIN  1          // PB19 : UART2_RX  (W601)
#define WM_UART2_TX_PIN  2          // PB20 : UART2_TX  (W601)   

#define WM_UART1_RX_PIN  31         // PB11 : UART1_RX  (W600)
#define WM_UART1_TX_PIN  32         // PB12 : UART1_TX  (W600)

// spi
#define WM_SPI_CK_PIN 53            // PB27 : SPI_SCK
#define WM_SPI_DI_PIN 55            // PB1  : SPI_MISO
#define WM_SPI_DO_PIN 56            // PB2  : SPI_MOSI

// i2c
#define SOFT_I2C1_SCL_PIN 23         // PA0 : I2C1_SCL     
#define SOFT_I2C1_SDA_PIN 24         // PA1 : I2C1_SDA   
#define SOFT_I2C2_SCL_PIN 25         // PA2 : I2C2_SCL     
#define SOFT_I2C2_SDA_PIN 24         // PA1 : I2C2_SDA   

// // ATK MODULE
// #define PIN_GBC_LED   97        // PE0 :  GBC_LED      --> ATK MODULE
// #define PIN_GBC_KEY   98        // PE1 :  GBC_KEY      --> ATK MODULE
// #define PIN_GBC_RX    25        // PA2 :  GBC_RX       --> ATK MODULE
// #define PIN_GBC_TX    26        // PA3 :  GBC_TX       --> ATK MODULE

// BEEP && LED && KEY
#define PIN_BEEP      45        // PB15:  BEEP         --> BEEP
#define PIN_LED_R     30        // PE7 :  LED_R        --> LED
#define PIN_LED_G     31        // PE8 :  LED_B        --> LED
#define PIN_LED_B     32        // PE9 :  LED_G        --> LED

#define PIN_KEY2      9999      //
#define PIN_KEY1      33        // PA6 :  KEY1         --> KEY
#define PIN_KEY0      35        // PA7 :  KEY0         --> KEY
#define PIN_WK_UP     36        // PA8 :  WK_UP        --> KEY

// INFRARED
#define PIN_EMISSION  37        // PA9 :  EMISSION     --> INFRARED EMISSION
#define PIN_RECEPTION 38        // PA10:  RECEPTION    --> INFRARED RECEPTION

// SENSOR
#define PIN_AP_INT    26        // PA2 :  AP_INT       --> ALS&PS SENSOR

// spi0 cs
#define PIN_SD_CS     64        // PB9 :  SD_CS        --> SD_CARD
#define PIN_LCD_CS    54        // PB0 :  LCD_CS       --> LCD
#define PIN_FLASH_CS  47        // PB16:  FLASH_CS     --> FLASH

// LCD
#define LCD_PWR_PIN 63
#define LCD_DC_PIN  57
#define LCD_RES_PIN 60

#endif /* __PIN_CONFIG_H__ */

