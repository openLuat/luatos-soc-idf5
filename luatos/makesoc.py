#!/usr/bin/python3
# -*- coding: UTF-8 -*-

import os
import shutil
import re
import json
import csv
import zipfile
import argparse
parser = argparse.ArgumentParser()
parser.add_argument('config_json', type=str, help='the config_json of idf')

# 打包路径
out_path = os.path.dirname(os.path.abspath(__file__))
pack_path = os.path.join(out_path,"pack")

# bin路径
bootloader_bin = os.path.join(out_path,"build","bootloader","bootloader.bin")
partition_table_bin = os.path.join(out_path,"build","partition_table","partition-table.bin")
luatos_bin = os.path.join(out_path,"build","luatos.bin")

luat_conf_bsp = os.path.join(out_path,"include","luat_conf_bsp.h")
info_json = os.path.join(pack_path,"info.json")
soc_download_exe = os.path.join(pack_path,"soc_download.exe")

def zip_dir(dirname,zipfilename):
    filelist = []
    for root, dirs, files in os.walk(dirname):
        for name in files:
            filelist.append(os.path.join(root, name))
    zf = zipfile.ZipFile(zipfilename, "w", compression=zipfile.ZIP_LZMA)
    for tar in filelist:
        arcname = tar[len(dirname):]
        zf.write(tar,arcname)
    zf.close()

if __name__=='__main__':
    args = parser.parse_args()
    config_json = args.config_json

    with open(config_json) as f :
        config_json_data = json.load(f)
        bsp = config_json_data["IDF_TARGET"].upper()
        partition_table_name = config_json_data["PARTITION_TABLE_FILENAME"]
        partition_table_csv = os.path.join(out_path,partition_table_name)

    script_size = "0x20000"

    with open(partition_table_csv) as f :
        reader = csv.DictReader(f)
        for row in reader:
            if row['# Name']=="app0":
                core_addr = row[' Offset']
            elif row['# Name']=="script":
                script_addr = row[' Offset']
                script_size = row['  Size']
            elif row['# Name']=="spiffs":
                fs_addr = row[' Offset']

    try :
        script_size = str(script_size).strip()
        if script_size.endswith("k"):
            script_size = int(script_size[:-2])
        else :
            script_size = int(script_size, 16) / 1024
    except:
        script_size = 128
        import traceback
        traceback.print_exc()
    print("rom size " + str(script_size) + "k")
    print("rom addr " + str(script_addr))
    vm_64bit = False
    with open(os.path.join(out_path,"include","luat_conf_bsp.h"), "r", encoding="UTF-8") as f :
        for line in f.readlines():                          #依次读取每行  
            version_data = re.findall(r'#define LUAT_BSP_VERSION "(.+?)"', line)#[0]
            if version_data:
                bsp_version = version_data[0]
            if line == "#define LUAT_CONF_VM_64bit\r" or line == "#define LUAT_CONF_VM_64bit\n":
                vm_64bit = True

    out_file_name="LuatOS-SoC_{}_{}".format(bsp_version, bsp)
    out_file = os.path.join(out_path,out_file_name)

    temp = os.path.join(pack_path,"temp")
    if os.path.exists(temp):
        shutil.rmtree(temp)
    os.mkdir(temp)
    shutil.copy(bootloader_bin, temp)
    shutil.copy(partition_table_bin, temp)
    shutil.copy(luatos_bin, temp)
    shutil.copy(luat_conf_bsp, temp)
    shutil.copy(info_json, temp)
    shutil.copy(soc_download_exe, temp)
    info_json_temp = os.path.join(temp,"info.json")

    with open(info_json_temp, "r") as f :
        info_json_data = json.load(f)
    with open(info_json_temp, "w") as f :
        print("script_size", script_size)
        if "LUAT_USE_ROMFS" in os.environ:
            print("USE ROMFS for script. Pls use LuaTools 2.2.3 +")
            info_json_data["rom"]["fs"]["script"]["type"] = "romfs"
        info_json_data["rom"]["fs"]["script"]["size"] = script_size
        info_json_data["download"]["core_addr"] = core_addr.replace("0x", "00").strip()
        info_json_data["download"]["script_addr"] = script_addr.replace("0x", "00").strip()
        info_json_data["download"]["fs_addr"] = fs_addr.replace("0x", "00").strip()
        if bsp == "ESP32C3":
            info_json_data["download"]["extra_param"] = "00ff0200"
        elif bsp == "ESP32S3":
            info_json_data["download"]["extra_param"] = "01ff0200"
        elif bsp == "ESP32":
            info_json_data["download"]["extra_param"] = "02ff0200"
        elif bsp == "ESP32C2":
            info_json_data["download"]["extra_param"] = "03ff0200"
        if vm_64bit:
            info_json_data["script"]["bitw"] = 64
        json.dump(info_json_data, f)
        print(json.dumps(info_json_data))
    
    if os.path.exists(out_file+'.soc'):
        os.remove(out_file+'.soc')
    if os.path.exists(out_file+'_USB.soc'):
        os.remove(out_file+'_USB.soc')
    
    # 首先, 生成不带USB后缀的soc文件
    zip_dir(temp, out_file+'.soc') 
    if bsp == "ESP32C3":
        # 然后生成USB版本的soc文件
        with open(info_json_temp, "w") as f :
            info_json_data["download"]["force_br"] = "0"
            json.dump(info_json_data, f)
        zip_dir(temp, out_file+'_USB.soc') 
    
    shutil.rmtree(temp)

    print(__file__,'done')
