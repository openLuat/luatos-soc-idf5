
import os
import sys
import shutil
import urllib
import re

bootloader_bin = './build/bootloader/bootloader.bin'
partition_table_bin = './build/partition_table/partition-table.bin'
luatos_bin = './build/luatos.bin'

out_path='./'
pack_path='./pack'

bsp='ESP32C3'

cwd_path = os.getcwd()
if __name__=='__main__':

    fo = open("./include/luat_conf_bsp.h", "r")
    for line in fo.readlines():                          #依次读取每行  
        find_data = re.findall(r'#define LUAT_BSP_VERSION "(.+?)"', line)#[0]
        if find_data:
            bsp_version = find_data[0]
    fo.close()# 关闭文件
    out_file="LuatOS-SoC_{}_{}".format(bsp_version, bsp)

    print(cwd_path)

    if os.path.exists(out_path+out_file+'.soc'):
        os.remove(out_path+out_file+'.soc')

    shutil.copy(bootloader_bin, pack_path+'/')
    shutil.copy(partition_table_bin, pack_path+'/')
    shutil.copy(luatos_bin, pack_path+'/')

    shutil.make_archive(out_path+out_file, 'zip', root_dir=pack_path)
    os.rename(out_path+out_file+'.zip',out_file+'.soc')
    
    print('end')
