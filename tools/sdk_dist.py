import os
import sys
cwd_path = os.getcwd()
sys.path.append(os.path.join(os.path.dirname(cwd_path), 'rt-thread', 'tools'))

def bsp_update_driver_kconfig(dist_dir):
    # change RTT_ROOT in Kconfig
    if not os.path.isfile(os.path.join(dist_dir, 'Kconfig')):
        return

    with open(os.path.join(dist_dir, 'Kconfig'), 'r') as f:
        data = f.readlines()
    with open(os.path.join(dist_dir, 'Kconfig'), 'w') as f:
        found = 0
        for line in data:
            if line.find('RTT_ROOT') != -1:
                found = 1
            if line.find('source "$BSP_DIR/../../drivers') != -1 and found:
                position = line.find('source "$BSP_DIR/../../drivers')
                line = line[0:position] + 'source "$BSP_DIR/drivers/Kconfig"\n'
                found = 0
            f.write(line)

def bsp_update_library_kconfig(dist_dir):
    # change RTT_ROOT in Kconfig
    if not os.path.isfile(os.path.join(dist_dir, 'Kconfig')):
        return

    with open(os.path.join(dist_dir, 'Kconfig'), 'r') as f:
        data = f.readlines()
    with open(os.path.join(dist_dir, 'Kconfig'), 'w') as f:
        found = 0
        for line in data:
            if line.find('RTT_ROOT') != -1:
                found = 1
            if line.find('source "$BSP_DIR/../../libraries') != -1 and found:
                position = line.find('source "$BSP_DIR/../../libraries')
                line = line[0:position] + 'source "$BSP_DIR/libraries/Kconfig"\n'
                found = 0
            f.write(line)

def bsp_update_template(template, replace_des, replace_src):
    try:
        if os.path.isfile(template):
            with open(template, 'r+') as f:
                file_content = f.read()

            file_content = file_content.replace(replace_src, replace_des)

            with open(template, 'w') as f:
                f.write(str(file_content))
    except Exception, e:
        print('e.message:%s\t' % e.message)

# BSP dist function
def dist_do_building(BSP_ROOT):
    from mkdist import bsp_copy_files

    dist_dir  = os.path.join(BSP_ROOT, 'dist', os.path.basename(BSP_ROOT))
    print("=> copy bsp library")
    library_path = os.path.join(os.path.dirname(os.path.dirname(BSP_ROOT)), 'libraries')
    library_dir  = os.path.join(dist_dir, 'libraries')
    bsp_copy_files(library_path, library_dir)

    print("=> copy bsp drivers")
    driver_path  = os.path.join(os.path.dirname(os.path.dirname(BSP_ROOT)), 'drivers')
    driver_dir   = os.path.join(dist_dir, 'drivers')
    bsp_copy_files(driver_path, driver_dir)

    print("=> copy tools")
    tools_path  = os.path.join(os.path.dirname(os.path.dirname(BSP_ROOT)), 'tools')
    tools_dir   = os.path.join(dist_dir, 'tools')
    bsp_copy_files(tools_path, tools_dir)

    iar_template_file =  os.path.join(dist_dir, 'template.ewp')
    bsp_update_template(iar_template_file, '$PROJ_DIR$\\drivers\\linker_scripts', '$PROJ_DIR$\\..\\..\\drivers\\linker_scripts')

    mdk_template_file =  os.path.join(dist_dir, 'template.uvprojx')
    bsp_update_template(mdk_template_file, '<ScatterFile>.\\drivers\\linker_scripts\\link.sct</ScatterFile>', '<ScatterFile>..\\..\\drivers\\linker_scripts\\link.sct</ScatterFile>')

    makimg_file =  os.path.join(dist_dir, 'makeimg.bat')
    bsp_update_template(makimg_file, 'set wmlib_path=.\\libraries\\WM_Libraries', 'set wmlib_path=..\\..\\libraries\\WM_Libraries')
    bsp_update_template(makimg_file, 'set tools_path=.\\tools', 'set tools_path=..\\..\\tools')

    makimg_file =  os.path.join(dist_dir, 'makeimg.py')
    bsp_update_template(makimg_file, 'wmlib_path=\'./libraries/WM_Libraries\'', 'wmlib_path=\'../../libraries/WM_Libraries\'')
    bsp_update_template(makimg_file, 'tools_path=\'./tools\'', 'tools_path=\'../../tools\'')

    bsp_update_driver_kconfig(dist_dir)
    bsp_update_library_kconfig(dist_dir)
