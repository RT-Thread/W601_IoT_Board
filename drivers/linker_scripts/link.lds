/* Linker script to configure memory regions. 
 * Need modifying for a specific board. 
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x8010100, LENGTH = 959K    /* 960K-256B */
    RAM (rw) : ORIGIN = 0x20000000, LENGTH = 0x28000  /* 160K */
    RAM2 (rw) : ORIGIN = 0x20028000, LENGTH = 0x14000 /* 80K */
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 * 
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        _stext = .;
        KEEP(*(.isr_vector))
        . = ALIGN(4);

        *(.text)                        /* remaining code */
        *(.text.*)                      /* remaining code */
        *(.rodata)                      /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t*)
        
        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;

        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;

        /* section information for initial. */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;

        . = ALIGN(4);
        __rtatcmdtab_start = .;
        KEEP(*(RtAtCmdTab))
        __rtatcmdtab_end = .;
        . = ALIGN(4);

        . = ALIGN(4);
        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)

        KEEP(*(.eh_frame*))
    } > FLASH

    .ARM.extab : 
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    __etext = .;

    .data : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(vtable)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > RAM

    .stack_dummy (COPY):
    {
        . = ALIGN(8);
        __stack_start__ = .;
        *(.stack*)
        __stack_end__ = .;
    } > RAM

    ._bss :
    {
        . = ALIGN(8);
        __bss2_start__ = .;        
        *memp.o (.bss*)
        *wm_param.o (.bss*)
        *wm_fwup.o (.bss*)
        *tls_rf_tx_iq.o (.bss*)
        *tx_lo_cal.o (.bss*)
        *ef_env.o (.bss*)
        *dns.o (.bss*)
        /* Other bss data */
        *kservice.o (.bss*)
        *mem.o (.bss*)
        *scheduler.o (.bss*)
        *ntp.o (.bss*)
        *dfs.o (.bss*)
        *dfs_file.o (.bss*)
        *pin.o (.bss*)
        *soft_rtc.o (.bss*)
        *spi_flash_sfud.o (.bss*)
        *sfud.o (.bss*)
        *wlan_cfg.o (.bss*)
        *wlan_lwip.o (.bss*)
        *wlan_mgnt.o (.bss*)
        *wlan_prot.o (.bss*)
        *shell.o (.bss*)
        *netdb.o (.bss*)
        *sockets.o (.bss*)
        *ip.o (.bss*)
        *tcp_in.o (.bss*)
        *dhcp.o (.bss*)
        *etharp.o (.bss*)
        *ip4_addr.o (.bss*)
        *netdev_ipaddr.o (.bss*)
        *sal_socket.o (.bss*)
        *wm_gpio.o (.bss*)
        *wm_irq.o (.bss*)
        *wm_efuse.o (.bss*)
        *wm_dma.o (.bss*)
        *wm_timer.o (.bss*)
        *wm_i2c.o (.bss*)
        *wm_adc.o (.bss*)
        *rtthread_patch.o (.bss*)
        *drv_uart.o (.bss*)
        *drv_wifi.o (.bss*)
        *drv_spi.o (.bss*)
        *smartconfig.o (.bss*)
        *random.o (.bss*)
        *tls_wl_util.o (.bss*)
        *common.o (.bss*)
        *stdio_streams.o (.bss*)
        *rand.o (.bss*)
        *localtime.o (.bss*)
        *asctime.o (.bss*)
        *libspace.o (.bss*)
        __bss2_end__ = .;
    } > RAM2

    .bss __stack_end__ :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM 

    ASSERT(__StackTop <= 0x20028000, "stack address error")
}
