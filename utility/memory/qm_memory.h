/** @file    qm_memory.h
 *  @author  Wells
 *  @version 1.0
 *  @date    25-Apr-2016
 *  @brief  
 */

#ifndef _QM_MEMORY_H_
#define _QM_MEMORY_H_

#ifdef	__cplusplus
	extern "C" {
#endif

#include "qm_types.h"
#include "qm_config.h"

/* Block sizes must not get too small. */
#define CONFIG_QM_MEM_MINI_BLOCK_SIZE	( ( uint32_t ) ( xHeapStructSize * 2 ) )
/* Assumes 8bit bytes! */
#define CONFIG_QM_MEM_BITS_PER_BYTE		( ( uint32_t ) 8 )

#ifndef CONFIG_QM_MEM_HEAP_SIZE
#define CONFIG_QM_MEM_HEAP_SIZE       (4096)
#endif

#ifndef CONFIG_QM_MEM_BYTE_ALIGNMENT
#define CONFIG_QM_MEM_BYTE_ALIGNMENT       4
#endif

#if CONFIG_QM_MEM_BYTE_ALIGNMENT == 8
#define CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK 	( 0x0007U )
#endif

#if CONFIG_QM_MEM_BYTE_ALIGNMENT == 4
#define CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK	( 0x0003 )
#endif

#if CONFIG_QM_MEM_BYTE_ALIGNMENT == 2
#define CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK	( 0x0001 )
#endif

#if CONFIG_QM_MEM_BYTE_ALIGNMENT == 1
#define CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK	( 0x0000 )
#endif

#if CONFIG_QM_MEMORY_SUPPORT
void qm_heap_init( uint8_t* heap, uint32_t heap_len);
void *qm_malloc( unsigned int xWantedSize );
void qm_free( void *pv );
void *qm_calloc(uint32_t num, uint32_t size);
uint32_t qm_free_mem_get( void );
#endif

#ifdef	__cplusplus
}
#endif
	

#endif
