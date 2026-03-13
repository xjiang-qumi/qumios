/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __QM_SPEC_API_H__
#define __QM_SPEC_API_H__

/* Includes ------------------------------------------------------------------*/
#include "qm.h"

/** @brief Spec API version. */
#define QM_SPEC_VERSION                     1

/** @brief Max string length for spec. */
#define QM_SPEC_LENGTH_STRING               1024

/** @brief Max number of arguments. */
#define QM_SPEC_MAX_ARGUMENTS               12

/** @brief Spec buffer size. */
#define CONFIG_QM_SPEC_BUFF_SIZE            (8)

/**
 * @brief Property value format type.
 */
typedef enum {
    QM_SPEC_PROPERTY_FORMAT_BOOL = 0,
    QM_SPEC_PROPERTY_FORMAT_INT8,
    QM_SPEC_PROPERTY_FORMAT_UINT8,
    QM_SPEC_PROPERTY_FORMAT_INT16,
    QM_SPEC_PROPERTY_FORMAT_UINT16,
    QM_SPEC_PROPERTY_FORMAT_INT32,
    QM_SPEC_PROPERTY_FORMAT_UINT32,
    QM_SPEC_PROPERTY_FORMAT_INT64,
    QM_SPEC_PROPERTY_FORMAT_UINT64,
    QM_SPEC_PROPERTY_FORMAT_FLOAT32,
    QM_SPEC_PROPERTY_FORMAT_FLOAT64,
    QM_SPEC_PROPERTY_FORMAT_STRING,
    QM_SPEC_PROPERTY_FORMAT_DATE,
    QM_SPEC_PROPERTY_FORMAT_STRUCT,
    QM_SPEC_PROPERTY_FORMAT_ARRAY,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_ONE_UINT16,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_TWO_UINT16,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_ONE_UINT32,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_TWO_UINT32,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_ONE_INT16,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_TWO_INT16,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_ONE_INT32,
    QM_SPEC_PROPERTY_FORMAT_FLOAT_TWO_INT32,
    QM_SPEC_PROPERTY_FORMAT_GROUP ,
    QM_SPEC_PROPERTY_FORMAT_STRING_ARRAY,
    QM_SPEC_PROPERTY_FORMAT_NUMBER,
    QM_SPEC_PROPERTY_FORMAT_NONE = 0xFF,
}qm_spec_property_format_t;

/**
 * @brief Property value (length, format, and value bytes or pointer).
 */
 typedef struct 
{
    uint16_t            len;             /**< Value length. */
    uint8_t            property_format;  /**< Format, @see qm_spec_property_format_t. */
    union 
    {
        uint8_t *pdata;                  /**< Pointer to data. */
        uint8_t bytes[CONFIG_QM_SPEC_BUFF_SIZE];  /**< Inline bytes. */
    }value;
}qm_spec_property_value_t;

/**
 * @brief Single argument (piid + value).
 */
 typedef struct {
    uint16_t piid;                       /**< Property ID. */
    qm_spec_property_value_t value;      /**< Property value. */
}argument_t;

/**
 * @brief Argument list for actions/events.
 */
 typedef struct {
    uint16_t      arguments_num;         /**< Number of arguments. */
    argument_t    arguments[QM_SPEC_MAX_ARGUMENTS];  /**< Arguments. */
} arguments_t;

/**
 * @brief Spec property node (siid, piid, code, value, next).
 */
 typedef struct _spec_property_t
{
#ifndef CONFIG_CANCEL_QM_SPEC_CJSON_SUPPORT
    uint32_t                      did;   /**< Device ID (optional). */
#endif
    uint8_t                     siid;   /**< Service ID. */
    uint16_t                    piid;   /**< Property ID. */
    int8_t                      code;   /**< Status code. */
    qm_spec_property_value_t    value;  /**< Property value. */
    struct _spec_property_t     *next;   /**< Next property in list. */
}qm_spec_property_t;

/**
 * @brief Property operation (list of properties).
 */
 typedef struct
{
    uint16_t              element_num;   /**< Number of properties. */
    struct _spec_property_t *property;   /**< Property list head. */
}qm_spec_property_operation_t;

/**
 * @brief Action operation (siid, aiid, in/out arguments).
 */
typedef struct 
{
    uint8_t             siid;            /**< Service ID. */
    uint16_t            aiid;            /**< Action ID. */
    int8_t              code;             /**< Status code. */
    arguments_t         *in;             /**< Input arguments. */
    arguments_t         *out;            /**< Output arguments. */
}qm_spec_action_operation_t;

/**
 * @brief Event operation (siid, eiid, arguments).
 */
typedef struct 
{
    uint8_t             siid;            /**< Service ID. */
    uint16_t            eiid;            /**< Event ID. */
    arguments_t         *arguments;     /**< Event arguments. */
}qm_spec_event_operation_t;

/**
 * @brief Create an empty property operation.
 * @return New property operation, or NULL on failure.
 */
qm_spec_property_operation_t *qm_spec_property_operation_creat(void);
/**
 * @brief Free a property operation and its properties.
 * @param property_operation  Operation to free.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_operation_delete(qm_spec_property_operation_t *property_operation);
/**
 * @brief Merge src property operation into dst (properties moved to dst).
 * @param dst_operation  Destination.
 * @param src_operation  Source (will be emptied).
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_operation_merge(qm_spec_property_operation_t *dst_operation, qm_spec_property_operation_t *src_operation);

/**
 * @brief Create an empty property node.
 * @return New property, or NULL on failure.
 */
qm_spec_property_t *qm_spec_property_creat(void);
/**
 * @brief Free a property node.
 * @param property_element  Property to free.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_delete(qm_spec_property_t *property_element);

/**
 * @brief Add a property to a property operation.
 * @param property_operation  Operation to modify.
 * @param property            Property to add.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_add(qm_spec_property_operation_t *property_operation, qm_spec_property_t *property);
/**
 * @brief Remove a property from a property operation.
 * @param property_operation  Operation to modify.
 * @param property            Property to remove.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_remove(qm_spec_property_operation_t *property_operation, qm_spec_property_t *property);

/**
 * @brief Find property by siid and piid.
 * @param property_operation  Operation to search.
 * @param siid  Service ID.
 * @param piid  Property ID.
 * @return Property if found, NULL otherwise.
 */
qm_spec_property_t *qm_spec_property_find(qm_spec_property_operation_t *property_operation, uint8_t siid, uint16_t piid);
/**
 * @brief Get next property in operation after the given one.
 * @param property_operation  Operation.
 * @param property            Current property.
 * @return Next property, or NULL.
 */
qm_spec_property_t *qm_spec_property_next(qm_spec_property_operation_t *property_operation, qm_spec_property_t *property);

/**
 * @brief Copy property content from src to dst.
 * @param dst_property  Destination.
 * @param src_property  Source.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_copy(qm_spec_property_t *dst_property, const qm_spec_property_t *src_property);

/**
 * @brief Pack siid and piid into property (xiid).
 * @param property  Property to set.
 * @param siid     Service ID.
 * @param piid     Property ID.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_pack_xiid(qm_spec_property_t *property, uint8_t siid, uint16_t piid);
/**
 * @brief Unpack xiid from property into siid and piid.
 * @param property  Property to read.
 * @param siid     [OUT] Service ID.
 * @param piid     [OUT] Property ID.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_unpack_xiid(qm_spec_property_t *property, uint8_t *siid, uint16_t *piid);

/**
 * @brief Get byte length for a property format.
 * @param format  Format type.
 * @return Length in bytes.
 */
int qm_spec_property_data_format_len(qm_spec_property_format_t format);

/**
 * @brief Pack a number into property value.
 * @param property  Property to set.
 * @param value     Number value.
 * @return 0 on success, negative on failure.
 */
int qm_spec_property_pack_number(qm_spec_property_t *property, int value);

/**
 * @brief Pack bool into property.
 */
int qm_spec_property_pack_bool(qm_spec_property_t *property, bool_t value);
/**
 * @brief Pack uint8 into property.
 */
int qm_spec_property_pack_uint8(qm_spec_property_t *property, uint8_t value);
/**
 * @brief Pack int8 into property.
 */
int qm_spec_property_pack_int8(qm_spec_property_t *property, int8_t value);
/**
 * @brief Pack uint16 into property.
 */
int qm_spec_property_pack_uint16(qm_spec_property_t *property, uint16_t value);
/**
 * @brief Pack int16 into property.
 */
int qm_spec_property_pack_int16(qm_spec_property_t *property, int16_t value);
/**
 * @brief Pack uint32 into property.
 */
int qm_spec_property_pack_uint32(qm_spec_property_t *property, uint32_t value);
/**
 * @brief Pack int32 into property.
 */
int qm_spec_property_pack_int32(qm_spec_property_t *property, int32_t value);
/**
 * @brief Pack float into property.
 */
int qm_spec_property_pack_float32(qm_spec_property_t *property, float value);
/**
 * @brief Pack raw bytes into property.
 */
int qm_spec_property_pack_bytes(qm_spec_property_t *property, uint8_t *bytes, int size);
/**
 * @brief Pack string into property.
 */
int qm_spec_property_pack_string(qm_spec_property_t *property, char *str, int size);

/**
 * @brief Unpack number from property.
 */
int qm_spec_property_unpack_number(qm_spec_property_t *property, int *value);

/**
 * @brief Unpack bool from property.
 */
int qm_spec_property_unpack_bool(qm_spec_property_t *property, bool_t *value);
/**
 * @brief Unpack uint8 from property.
 */
int qm_spec_property_unpack_uint8(qm_spec_property_t *property, uint8_t *value);
/**
 * @brief Unpack int8 from property.
 */
int qm_spec_property_unpack_int8(qm_spec_property_t *property, int8_t *value);
/**
 * @brief Unpack uint16 from property.
 */
int qm_spec_property_unpack_uint16(qm_spec_property_t *property, uint16_t *value);
/**
 * @brief Unpack int16 from property.
 */
int qm_spec_property_unpack_int16(qm_spec_property_t *property, int16_t *value);
/**
 * @brief Unpack uint32 from property.
 */
int qm_spec_property_unpack_uint32(qm_spec_property_t *property, uint32_t *value);
/**
 * @brief Unpack int32 from property.
 */
int qm_spec_property_unpack_int32(qm_spec_property_t *property, int32_t *value);
/**
 * @brief Unpack float from property.
 */
int qm_spec_property_unpack_float32(qm_spec_property_t *property, float *value);
/**
 * @brief Unpack bytes from property into buffer.
 */
int qm_spec_property_unpack_bytes(qm_spec_property_t *property, uint8_t *bytes, int *size);
/**
 * @brief Unpack string from property into buffer.
 */
int qm_spec_property_unpack_string(qm_spec_property_t *property, char *str, int *size);
/**
 * @brief Unpack bytes from property (direct pointer to internal data).
 */
int qm_spec_property_unpack_bytes_direct(qm_spec_property_t *property, uint8_t **bytes, int *size);
/**
 * @brief Unpack string from property (direct pointer to internal data).
 */
int qm_spec_property_unpack_string_direct(qm_spec_property_t *property, char **str, int *size);

#endif
