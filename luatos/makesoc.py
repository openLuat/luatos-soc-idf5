#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import shutil
import re
import json
import zlib

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('target', type=str, help='the target of idf')

out_path = os.path.dirname(os.path.abspath(__file__))
pack_path = os.path.join(out_path,"pack")

bootloader_bin = os.path.join(out_path,"build","bootloader","bootloader.bin")
partition_table_bin = os.path.join(out_path,"build","partition_table","partition-table.bin")
luatos_bin = os.path.join(out_path,"build","luatos.bin")

if __name__=='__main__':
    args = parser.parse_args()
    bsp = args.target.upper()

    fo = open(os.path.join(out_path,"include","luat_conf_bsp.h"), "r", encoding="UTF-8")
    for line in fo.readlines():                          #依次读取每行  
        find_data = re.findall(r'#define LUAT_BSP_VERSION "(.+?)"', line)#[0]
        if find_data:
            bsp_version = find_data[0]
    fo.close()# 关闭文件
    out_file="LuatOS-SoC_{}_{}".format(bsp_version, bsp)

    if os.path.exists(out_path+"\\"+out_file+'.soc'):
        os.remove(out_path+"\\"+out_file+'.soc')
    if os.path.exists(out_path+"\\"+out_file+'_USB.soc'):
        os.remove(out_path+"\\"+out_file+'_USB.soc')

    with open(pack_path + "/info.json", "rb") as f :
        fdata = f.read()
    with open(pack_path + "/info.json") as f :
        info_json = json.load(f)

    # 首先, 生成不带USB后缀的soc文件
    shutil.copy(bootloader_bin, pack_path+'/')
    shutil.copy(partition_table_bin, pack_path+'/')
    shutil.copy(luatos_bin, pack_path+'/')

    shutil.make_archive(out_path+"\\"+out_file, 'zip', root_dir=pack_path)
    os.rename(out_path+"\\"+out_file+'.zip',out_path+"\\"+out_file+'.soc')

    # 然后生成USB版本的soc文件
    with open(pack_path + "/info.json", "w") as f :
        info_json["download"]["force_br"] = "0"
        json.dump(info_json, f)
    shutil.make_archive(out_path+"\\"+out_file, 'zip', root_dir=pack_path)
    os.rename(out_path+"\\"+out_file+'.zip',out_path+"\\"+out_file+'_USB.soc')

    # 还原info.json
    with open(pack_path + "/info.json", "wb") as f :
        f.write(fdata)
    
    print('done', out_file)
