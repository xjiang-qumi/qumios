#include "qm_ota.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "qm_log.h"


#define platform_log(M, ...)       QM_LOGD("ota", M, ##__VA_ARGS__)

#define MAX_BIN_SIZE 500

static uint32_t offset_addr = 0;
static esp_partition_t  operate_partition;
static esp_ota_handle_t out_handle;
static esp_err_t esp_write_error;

static bool esp_ota_prepare()
{
    esp_err_t err;

    offset_addr = 0;
    
    const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();

    if (esp_current_partition == NULL) {
        platform_log("esp_ota_get_boot_partition got null prt");
        return false;
    }


    if (esp_current_partition->type != ESP_PARTITION_TYPE_APP) {
        platform_log("Error esp_current_partition->type != ESP_PARTITION_TYPE_APP");
        return false; 
    }

    esp_partition_t find_partition;

    /*choose which OTA image should we write to*/
    switch (esp_current_partition->subtype) {
        case ESP_PARTITION_SUBTYPE_APP_FACTORY:

            platform_log("esp_ota ESP_PARTITION_SUBTYPE_APP_FACTORY!");
            find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
            break;

        case  ESP_PARTITION_SUBTYPE_APP_OTA_0:

            platform_log("esp_ota ESP_PARTITION_SUBTYPE_APP_OTA_0!");
            find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
            break;

        case ESP_PARTITION_SUBTYPE_APP_OTA_1:

            platform_log("esp_ota ESP_PARTITION_SUBTYPE_APP_OTA_1!");
            find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
            break;

        default:

            platform_log("esp_ota cur partition=%d!",esp_current_partition->subtype);
            find_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
            break;
    }

    find_partition.type = ESP_PARTITION_TYPE_APP;

    const esp_partition_t *partition = esp_partition_find_first(find_partition.type, find_partition.subtype, NULL);
    assert(partition != NULL);
    memset(&operate_partition, 0, sizeof(esp_partition_t));

    platform_log("esp_ota_begin partition type 0x%x!", partition->type);
    platform_log("esp_ota_begin partition subtype 0x%x!", partition->subtype);
    platform_log("esp_ota_begin partition address 0x%x!", partition->address);
    platform_log("esp_ota_begin partition size 0x%x!", partition->size);
    platform_log("esp_ota_begin partition label %s!", partition->label);
    platform_log("esp_ota_begin partition encrypted 0x%x!", partition->encrypted);
    esp_write_error = ESP_OK;
    err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &out_handle);

    if (err != ESP_OK) {
        platform_log("esp_ota_begin failed err=0x%x!", err);
        return false;
    } else {
        memcpy(&operate_partition, partition, sizeof(esp_partition_t));
        platform_log("esp_ota_begin init OK");
        return true;
    }

    return false;
}


int qm_ota_start(void *arg)
{
        /* prepare to os update  */
    if (esp_ota_prepare() != true) { 
        return -1; 
    }
    return 0;
}

int qm_ota_write(uint32_t *off_set, uint8_t *in_buf, uint32_t in_buf_len)
{
    esp_err_t err = ESP_OK;

    if(off_set == NULL){
        err = esp_ota_write(out_handle, (const void *)in_buf, in_buf_len);
    }else{
        err = esp_ota_write_with_offset(out_handle, (const void *)in_buf, in_buf_len, offset_addr);
        offset_addr += in_buf_len;
    }

    if(err != ESP_OK) {
        esp_write_error = err;
        platform_log("Error: esp_ota_write failed! err=%x", err);
        return -1;
    }
    
    if(off_set){
        *off_set = offset_addr;
    }

    return 0;
}

int qm_ota_read(uint32_t *off_set, uint8_t *out_buf, uint32_t out_buf_len)
{
    size_t src_offset = 0;
    if(off_set == NULL || out_buf == NULL || out_buf_len == 0){
        return -QM_EINVAL;
    }

    src_offset = *off_set;
    esp_partition_read(&operate_partition, src_offset, out_buf, out_buf_len);

    *off_set += out_buf_len;
    
    return 0;
}

int qm_ota_end(void *arg)
{
    esp_err_t err = ESP_OK;
    if (esp_ota_end(out_handle) != ESP_OK) {
        platform_log("esp_ota_end failed!");
        return -1;
    }

    if(esp_write_error)  {
        platform_log("esp_ota_write_ota_cb write_error %d!",esp_write_error);
        return -1;
    }
    err = esp_ota_set_boot_partition(&operate_partition);

    if (err != ESP_OK) {
        platform_log("esp_ota_set_boot_partition failed! err=0x%x", err);
        return -1;
    }

    platform_log("Prepare to restart system!");
    esp_restart();

    return 0;
}
