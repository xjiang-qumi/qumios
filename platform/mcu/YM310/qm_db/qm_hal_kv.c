#include "qm.h"
#include "qm_hal_kv.h"
#include "yopen_fs.h"
#include "yopen_os.h"

#define QM_KV_STORAGE_NAMESPACE "qm_kv"

int qm_hal_kv_init(void)
{
    return QM_EOK;
}

int qm_hal_kv_get(const char *key, void *buffer, int *buffer_len)
{
    int ret = -1;
    int size =  0;
    QFILE fd = -1;

    if(key == NULL || buffer_len == NULL){
        return -QM_EINVAL;
    }

    // 检查文件是否存在
    if(yopen_file_exist(key) != 0) {
        QM_LOGD("qm_hal_kv", "key %s not exist", key);
        return -QM_ERROR;
    }

    // 打开文件
    fd = yopen_fopen(key, "rb");
    if(fd < YOPEN_FS_OK){
        QM_LOGD("qm_hal_kv", "can't open file %s", key);
        return -QM_ERROR;
    }

    // 获取文件大小
    size = yopen_fsize(fd);
    QM_LOGD("qm_hal_kv", "file size %d", size);

    *buffer_len = size;

    // 读取文件内容
    if(buffer && size > 0){
        ret = yopen_fread(buffer, 1, size, fd);
        if (ret != size){
            QM_LOGD("qm_hal_kv", "read file %s fail, ret %d", key, ret);
            yopen_fclose(fd);
            return -QM_ERROR;
        }
    }

    // 关闭文件
    yopen_fclose(fd);

    return QM_EOK;
}

int qm_hal_kv_set(const char *key, const void *value, int len, int sync)
{
    int ret = -1;
    QFILE fd = -1;

    if(key == NULL || value == NULL || len <= 0){
        return -QM_EINVAL;
    }

    // 如果文件已存在，先删除
    if(yopen_file_exist(key) == 0) {
        yopen_remove(key);
    }

    // 创建并打开文件
    fd = yopen_fopen(key, "wb+");
    if(fd < YOPEN_FS_OK){
        QM_LOGD("qm_hal_kv", "can't create file %s", key);
        return -QM_ERROR;
    }

    // 写入数据
    ret = yopen_fwrite(value, 1, len, fd);
    if (ret != len){
        QM_LOGD("qm_hal_kv", "write file %s fail, ret %d", key, ret);
        yopen_fclose(fd);
        return -QM_ERROR;
    }

    // 关闭文件
    yopen_fclose(fd);

    return QM_EOK;
}

int qm_hal_kv_del(const char *key)
{
    int ret = -1;

    if(key == NULL){
        return -QM_EINVAL;
    }

    // 检查文件是否存在
    if(yopen_file_exist(key) == 0) {
        // 删除文件
        ret = yopen_remove(key);
        if(ret != YOPEN_FS_OK) {
            QM_LOGD("qm_hal_kv", "delete file %s fail, ret %d", key, ret);
            return -QM_ERROR;
        }
    }

    return QM_EOK;
}

int qm_hal_kv_clean(void)
{
    QDIR* dir = NULL;
    qdirent *dir_info = NULL;

    // 打开根目录
    dir = yopen_opendir("/");
    if (dir == NULL) {
        QM_LOGD("qm_hal_kv", "can't open root directory");
        return -QM_ERROR;
    }

    // 遍历目录中的所有文件
    do {
        dir_info = yopen_readdir(dir);
        if (dir_info == NULL) {
            break;
        }

        // 只删除文件，不删除目录
        if (dir_info->d_type == YOPEN_FS_TYPE_FILE) {
            // 删除文件
            if (yopen_remove(dir_info->d_name) != YOPEN_FS_OK) {
                QM_LOGD("qm_hal_kv", "delete file %s fail", dir_info->d_name);
            }
        }

        // 释放目录项内存
        free(dir_info);

    } while (1);

    // 关闭目录
    yopen_closedir(dir);

    return QM_EOK;
}
