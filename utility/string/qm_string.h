#ifndef _QM_STRING_H_
#define _QM_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

char *qm_strdup(const char *s);
int32_t qm_str2uint(char *input, uint8_t input_len, uint32_t *output);
int32_t qm_str2uint64(char *input, uint8_t input_len, uint64_t *output);
int32_t qm_uint2str(uint32_t input, char *output, uint8_t *output_len);
int32_t qm_int2hexstr(int32_t input, char *output, uint8_t *output_len);
int32_t qm_uint642str(uint64_t input, char *output, uint8_t *output_len);
int32_t qm_int2str(int32_t input, char *output, uint8_t *output_len);
int32_t qm_hex2str(uint8_t *input, uint32_t input_len, char *output, uint8_t lowercase);
int32_t qm_str2hex(char *input, uint32_t input_len, uint8_t *output);
int32_t qm_json_value(const char *input, uint32_t input_len, const char *key, uint32_t key_len, char **value, uint32_t *value_len);
int32_t qm_utils_sprintf(char **dest, char *fmt, char *src[], uint8_t count, char *module_name);

#ifdef __cplusplus
}
#endif

#endif