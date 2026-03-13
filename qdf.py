#!/usr/bin/env python
#encoding=utf-8

import sys,getopt
import os
import shutil

def main(argv):
    option = sys.argv[1]
    qdf_path = os.environ.get("QDF_PATH")
    path = '%s/tools/kconfig/menuconfig.py' %qdf_path
    cur_work_dir  = os.getcwd()
    work_name = cur_work_dir.rsplit("/", 1)[-1]
    work_dir = '%s/apps/%s' %(qdf_path,work_name)
    
    if not os.path.exists("qm_config"):
        os.makedirs("qm_config")

    if option == "menuconfig":
        os.system('cd qm_config; python ${QDF_PATH}/tools/kconfig/menuconfig.py ${QDF_PATH}/qm_kconfig')
        if os.path.isfile('./qm_config/.config'):
            shutil.copy('./qm_config/.config', '%s' %qdf_path)
            os.system('cd ${QDF_PATH}; python ${QDF_PATH}/tools/kconfig/genconfig.py')
            os.remove('%s/.config' %qdf_path)
        if os.path.isfile('%s/config.h' %qdf_path):
            shutil.move('%s/config.h' %qdf_path, "./qm_config/qm_config.h")
    
    if option == "genconfig":
        if os.path.isfile('./qm_config/.config'):
            os.system('cd ${QDF_PATH};python ${QDF_PATH}/tools/kconfig/genconfig.py')
        if os.path.isfile('%s/config.h' %qdf_path):
            shutil.move('%s/config.h' %qdf_path, "./qm_config/qm_config.h")

if __name__ == '__main__':
    main(sys.argv[1:])








