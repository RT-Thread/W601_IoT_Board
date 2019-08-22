import rtconfig
Import('RTT_ROOT')
from building import *

# get current directory
cwd = GetCurrentDir()

# The set of source files associated with this SConscript file.
src = Split("""
    Platform/Drivers/cpu/wm_cpu.c
    Platform/Drivers/gpio/wm_gpio.c
    Platform/Drivers/io/wm_io.c
    Platform/Drivers/gpio/wm_gpio_afsel.c
    Platform/Drivers/irq/wm_irq.c
    Platform/Drivers/efuse/wm_efuse.c
    Platform/Drivers/internalflash/wm_internal_fls.c
    Platform/Drivers/spi/wm_hostspi.c
    Platform/Drivers/flash/wm_fls.c
    Platform/Drivers/flash/wm_fls_gd25qxx.c
    Platform/Drivers/dma/wm_dma.c
    Platform/Drivers/timer/wm_timer.c
    Platform/Drivers/watchdog/wm_watchdog.c
    Platform/Drivers/i2c/wm_i2c.c
    Platform/Drivers/pwm/wm_pwm.c
    Platform/Drivers/adc/wm_adc.c
    Platform/Drivers/pmu/wm_pmu.c
    Platform/Drivers/rtc/wm_rtc.c
""")

src += Split("""
    Platform/Common/Params/wm_param.c
""")

src += Split("""
    Platform/Common/fwup/wm_fwup.c
""")

src += Split("""
    Platform/Common/crypto/wm_crypto_hard.c
    Platform/Common/crypto/digest/wm_crypto_hmac.c
    Platform/Common/crypto/digest/wm_crypto_md2.c
    Platform/Common/crypto/digest/wm_crypto_md4.c
    Platform/Common/crypto/digest/sha224.c
    Platform/Common/crypto/digest/sha384.c
    Platform/Common/crypto/digest/wm_crypto_sha512.c
    Platform/Common/crypto/keyformat/asn1.c
    Platform/Common/crypto/keyformat/wm_crypto_base64.c
    Platform/Common/crypto/keyformat/wm_crypto_x509.c
    Platform/Common/crypto/math/pstm_mul_comba.c
    Platform/Common/crypto/prng/prng.c
    Platform/Common/crypto/prng/yarrow.c
    Platform/Common/crypto/pubkey/dh.c
    Platform/Common/crypto/pubkey/ecc.c
    Platform/Common/crypto/pubkey/pkcs.c
    Platform/Common/crypto/pubkey/pubkey.c
    Platform/Common/crypto/pubkey/wm_crypto_rsa.c
    Platform/Common/crypto/symmetric/aesGCM.c
    Platform/Common/crypto/symmetric/des3.c
    Platform/Common/crypto/symmetric/idea.c
    Platform/Common/crypto/symmetric/rc2.c
    Platform/Common/crypto/symmetric/seed.c 
""")

src += Split("""
    rtthread/rtthread_patch.c
    rtthread/utils/utils.c
""")

# add for startup script 
if rtconfig.CROSS_TOOL == 'keil':
    src += ['rtthread/startup/armcc/startup_venus.S']
    src += ['Platform/Boot/armcc/misc.c']
elif rtconfig.CROSS_TOOL == 'gcc':
    src += ['rtthread/startup/gcc/startup_venus.S']
    src += ['Platform/Boot/gcc/misc.c']
elif rtconfig.CROSS_TOOL == 'iar':
    src += ['rtthread/startup/iar/startup_venus.S']
    src += ['Platform/Boot/iar/misc.c']

path = [cwd,
        cwd + '/Include',
        cwd + '/Include/Driver',
        cwd + '/Platform/Drivers/spi',
        cwd + '/Include/OS',
        cwd + '/Include/Platform',
        cwd + '/Include/OS',
        cwd + '/Include/WiFi']

path += [cwd + '/Platform/Inc',
        cwd + '/Platform/Common/Params',
        cwd + '/Platform/Common/crypto',
        cwd + '/Platform/Common/crypto/digest',
        cwd + '/Platform/Common/crypto/math',
        cwd + '/Platform/Common/crypto/symmetric',
        cwd + '/Platform/Boot/gcc']

'''
if GetDepend(['RT_USING_BSP_CMSIS']):
    path += [cwd + '/CMSIS/CM3/CoreSupport']
    src += [cwd + '/CMSIS/CM3/CoreSupport/core_cm3.c']
elif GetDepend(['RT_USING_RTT_CMSIS']):
    path += [RTT_ROOT + '/components/CMSIS/Include']
'''

LIB_PATH = [cwd + '/Lib/Wlan']
if rtconfig.CROSS_TOOL == 'gcc':
    LIB = ['wlan_gcc']
elif rtconfig.CROSS_TOOL == 'keil':
    LIB = ['libwlan_mdk']
elif rtconfig.CROSS_TOOL == 'iar':
    LIB = ['libwlan_iar']

if GetDepend(['WM_USING_ONESHOT']):
    LIB_PATH += [cwd + '/Lib/oneshot']
    if rtconfig.CROSS_TOOL == 'gcc':
        LIB += ['wmoneshot_gcc']
    elif rtconfig.CROSS_TOOL == 'keil':
        LIB += ['libwmoneshot_mdk']
    elif rtconfig.CROSS_TOOL == 'iar':
        LIB += ['libwmoneshot_iar']

CPPDEFINES = ['WM_W600']
if rtconfig.CROSS_TOOL == 'iar':
    CPPDEFINES += ['__inline=inline']
    CPPDEFINES += ['__inline__=inline']

group = DefineGroup('Libraries', src, depend = [''], CPPPATH = path, CPPDEFINES = CPPDEFINES, LIBS = LIB, LIBPATH = LIB_PATH)

Return('group')
