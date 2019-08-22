import os
from building import * 

# get current dir path
cwd = GetCurrentDir()

# init src and inc vars
src = []
inc = []

# add librws common include
inc = inc + [cwd + '/librws/inc']

# add librws basic code
src = src + Glob('librws/src/*.c')

# add group to IDE project
objs = DefineGroup('librws_rtthread', src, depend = ['PKG_USING_LIBRWS', 'RT_USING_PTHREADS'], CPPPATH = inc)

# traversal subscript
list = os.listdir(cwd)
if GetDepend(['PKG_USING_LIBRWS', 'RT_USING_PTHREADS']):
    for d in list:
        path = os.path.join(cwd, d)
        if os.path.isfile(os.path.join(path, 'SConscript')):
            objs = objs + SConscript(os.path.join(d, 'SConscript'))

Return('objs')

