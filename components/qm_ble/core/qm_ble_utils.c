
#include "qm_ble_bzopt.h"
#include "qm_ble_utils.h"

uint8_t qm_ble_hex2ascii(uint8_t digit)
{
    uint8_t val;

    if (digit <= 9) {
        val = digit - 0x0 + '0';
    } else {
        val = digit - 0xA + 'A';
    }

    return val;
}

void qm_ble_hex2string(uint8_t *hex, uint32_t len, uint8_t *str)
{
    uint32_t index;

    for (index = 0; index < len; index++) {
        str[index * 2] = qm_ble_hex2ascii(hex[index] >> 4 & 0x0f);
        str[index * 2 + 1] = qm_ble_hex2ascii(hex[index] & 0x0f);
    }
}

static void hex_byte_dump(uint8_t *data, int len, int tab_num)
{
    int i;
    for (i = 0; i < len; i++) {
        qm_printf("%02x ", data[i]);

        if (!((i + 1) % tab_num)) {
            qm_printf("\r\n");
        }
    }

    qm_printf("\r\n");
}

void qm_ble_hex_byte_dump_debug(uint8_t *data, int len, int tab_num)
{
#if (CONFIG_BLDTIME_MUTE_DBGLOG)

#else
    hex_byte_dump(data, len, tab_num);
#endif
}

void qm_ble_hex_byte_dump_verbose(uint8_t *data, int len, int tab_num)
{
#if defined(QB_VERBOSE_DEBUG)
    hex_byte_dump(data, len, tab_num);
#else
    
#endif
}
