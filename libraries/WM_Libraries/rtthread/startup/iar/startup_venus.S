;********************************************************************************
;* @file      startup.s
;* @version   V1.00
;* @date      1/16/2019
;* @brief     CMSIS Cortex-M3 Core Device Startup File for the W60X
;*
;* @note      Copyright (C) 2019 WinnerMicro Inc. All rights reserved.
;*
;* <h2><center>&copy; COPYRIGHT 2019 WinnerMicro</center></h2>
;*
;********************************************************************************

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        PUBLIC  __vector_table

;*******************************************************************************
; Fill-up the Vector Table entries with the exceptions ISR address
;*******************************************************************************
        DATA

__vector_table

        DCD  sfe(CSTACK)                        ; Top address of Stack
        DCD  Reset_Handler                      ; Reset Handler
        DCD  NMI_Handler                        ; NMI Handler
        DCD  HardFault_Handler                  ; Hard Fault Handler
        DCD  MemManage_Handler                  ; Memory Management Fault Handler
        DCD  BusFault_Handler                   ; Bus Fault Handler
        DCD  UsageFault_Handler                 ; Usage Fault Handler
        DCD  0                                  ; Reserved
        DCD  0                                  ; Reserved
        DCD  0                                  ; Reserved
        DCD  0                                  ; Reserved
        DCD  SVC_Handler                        ; SVC Handler
        DCD  DebugMon_Handler                   ; Debug Monitor Handler
        DCD  0                                  ; Reserved
        DCD  PendSV_Handler                     ; PendSV Handler
        DCD  OS_CPU_SysTickHandler              ; SysTick Handler

        ; External Interrupt Handler
        DCD  SDIO_RX_IRQHandler
        DCD  SDIO_TX_IRQHandler
        DCD  SDIO_RX_CMD_IRQHandler
        DCD  SDIO_TX_CMD_IRQHandler
        DCD  tls_wl_mac_isr
        DCD  0
        DCD  tls_wl_rx_isr
        DCD  tls_wl_mgmt_tx_isr
        DCD  tls_wl_data_tx_isr
        DCD  PMU_TIMER1_IRQHandler
        DCD  PMU_TIMER0_IRQHandler
        DCD  PMU_GPIO_WAKE_IRQHandler
        DCD  PMU_SDIO_WAKE_IRQHandler
        DCD  DMA_Channel0_IRQHandler
        DCD  DMA_Channel1_IRQHandler
        DCD  DMA_Channel2_IRQHandler
        DCD  DMA_Channel3_IRQHandler
        DCD  DMA_Channel4_7_IRQHandler
        DCD  DMA_BRUST_IRQHandler
        DCD  I2C_IRQHandler
        DCD  ADC_IRQHandler
        DCD  SPI_LS_IRQHandler
        DCD  SPI_HS_IRQHandler
        DCD  UART0_IRQHandler
        DCD  UART1_IRQHandler
        DCD  GPIOA_IRQHandler
        DCD  TIM0_IRQHandler
        DCD  TIM1_IRQHandler
        DCD  TIM2_IRQHandler
        DCD  TIM3_IRQHandler
        DCD  TIM4_IRQHandler
        DCD  TIM5_IRQHandler
        DCD  WDG_IRQHandler
        DCD  PMU_IRQHandler
        DCD  FLASH_IRQHandler
        DCD  PWM_IRQHandler
        DCD  I2S_IRQHandler
        DCD  PMU_RTC_IRQHandler
        DCD  RSA_IRQHandler
        DCD  CRYPTION_IRQHandler
        DCD  GPIOB_IRQHandler
        DCD  UART2_IRQHandler
        DCD  0

        THUMB

        PUBWEAK Reset_Handler
        SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler
        LDR     R0, =__iar_program_start
        BX      R0

        PUBWEAK NMI_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
NMI_Handler
        B NMI_Handler

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Handler
        B HardFault_Handler

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
MemManage_Handler
        B MemManage_Handler

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
BusFault_Handler
        B BusFault_Handler

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UsageFault_Handler
        B UsageFault_Handler

        PUBWEAK SVC_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SVC_Handler
        B SVC_Handler

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DebugMon_Handler
        B DebugMon_Handler

;        PUBWEAK PendSV_Handler
;        SECTION .text:CODE:REORDER:NOROOT(1)
;PendSV_Handler
;        B PendSV_Handler
;
;        PUBWEAK SysTick_Handler
;        SECTION .text:CODE:REORDER:NOROOT(1)
;SysTick_Handler
;        B SysTick_Handler

        PUBWEAK PendSV_Handler
        PUBWEAK OS_CPU_SysTickHandler
        PUBWEAK SDIO_RX_IRQHandler
        PUBWEAK SDIO_TX_IRQHandler
        PUBWEAK SDIO_RX_CMD_IRQHandler
        PUBWEAK SDIO_TX_CMD_IRQHandler
        EXTERN tls_wl_mac_isr
        EXTERN tls_wl_rx_isr
        EXTERN tls_wl_data_tx_isr
        EXTERN tls_wl_mgmt_tx_isr
        EXTERN RSV_IRQHandler
        PUBWEAK PMU_RTC_IRQHandler
        EXTERN PMU_TIMER1_IRQHandler
        EXTERN PMU_TIMER0_IRQHandler
        EXTERN PMU_GPIO_WAKE_IRQHandler
        EXTERN PMU_SDIO_WAKE_IRQHandler
        EXTERN DMA_Channel0_IRQHandler
        EXTERN DMA_Channel1_IRQHandler
        EXTERN DMA_Channel2_IRQHandler
        EXTERN DMA_Channel3_IRQHandler
        EXTERN DMA_Channel4_7_IRQHandler
        PUBWEAK DMA_BRUST_IRQHandler
        EXTERN I2C_IRQHandler
        EXTERN ADC_IRQHandler
        EXTERN SPI_LS_IRQHandler
        PUBWEAK SPI_HS_IRQHandler
        EXTERN UART0_IRQHandler
        PUBWEAK UART1_IRQHandler
        EXTERN GPIOA_IRQHandler
        EXTERN TIM0_IRQHandler
        EXTERN TIM1_IRQHandler
        EXTERN TIM2_IRQHandler
        EXTERN TIM3_IRQHandler
        EXTERN TIM4_IRQHandler
        EXTERN TIM5_IRQHandler
        PUBWEAK WDG_IRQHandler
        PUBWEAK PMU_IRQHandler
        PUBWEAK FLASH_IRQHandler
        EXTERN PWM_IRQHandler
        PUBWEAK I2S_IRQHandler
        EXTERN PMU_6IRQHandler
        EXTERN RSA_IRQHandler
        EXTERN CRYPTION_IRQHandler
        EXTERN GPIOB_IRQHandler
        PUBWEAK UART2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
PendSV_Handler
OS_CPU_SysTickHandler
SDIO_RX_IRQHandler
SDIO_TX_IRQHandler
SDIO_RX_CMD_IRQHandler
SDIO_TX_CMD_IRQHandler
PMU_RTC_IRQHandler
DMA_BRUST_IRQHandler
SPI_HS_IRQHandler
UART1_IRQHandler
PMU_IRQHandler
FLASH_IRQHandler
I2S_IRQHandler
UART2_IRQHandler
WDG_IRQHandler
        B .

        END
