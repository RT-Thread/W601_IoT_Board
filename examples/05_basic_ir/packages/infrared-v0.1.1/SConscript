from building import *

cwd     = GetCurrentDir()
src     = []
path    = [cwd + '/inc']

src += ['src/infrared.c']

if GetDepend(['INFRARED_NEC_DECODER']):
    src += ['src/nec_decoder.c']

if GetDepend(['PKG_USING_DRV_INFRARED']):
    src += ['src/drv_infrared.c']

group = DefineGroup('Infrared_frame', src, depend = ['PKG_USING_INFRARED'], CPPPATH = path)

Return('group')
