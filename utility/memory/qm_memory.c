/** @file    qm_memory.c
 *  @author  Wells
 *  @version 1.0
 *  @date    25-Apr-2016
 *  @brief  
 */


#include "qm.h"
#if CONFIG_QM_MEMORY_SUPPORT
#include "qm_memory.h"

#if CONFIG_QM_OS_SUPPORT
static qm_mutex_t g_mem_lock = {0};
#endif
/* Allocate the memory for the heap. */
//static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	uint32_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */


/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const uint32_t xHeapStructSize	= ( ( sizeof( BlockLink_t ) + ( CONFIG_QM_MEM_BYTE_ALIGNMENT - 1 ) ) & ~CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static uint32_t xFreeBytesRemaining = 0U;
static uint32_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an uint32_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static uint32_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/

void *qm_malloc( unsigned int xWantedSize )
{
	void *pvReturn = NULL;
	BlockLink_t *pxBlock , *pxPreviousBlock, *pxNewBlockLink;
#if CONFIG_QM_OS_SUPPORT
 	qm_mutex_lock(&g_mem_lock, QM_WAIT_FOREVER);
#endif
	{

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( CONFIG_QM_MEM_BYTE_ALIGNMENT - ( xWantedSize & CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK ) );
				}
			
			}
		
			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > CONFIG_QM_MEM_MINI_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );
					
						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
		
					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
			

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
			
			}
		
		}

	}
#if CONFIG_QM_OS_SUPPORT
   qm_mutex_unlock(&g_mem_lock);
#endif


	return pvReturn;
}
/*-----------------------------------------------------------*/

void qm_free( void *pv )
{
	uint8_t *puc = ( uint8_t * ) pv;
	BlockLink_t *pxLink;
	
	if( pv != NULL )
	{
  
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */

		//configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		//configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;
			#if CONFIG_QM_OS_SUPPORT		
				qm_mutex_lock(&g_mem_lock, QM_WAIT_FOREVER);
			#endif
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
			#if CONFIG_QM_OS_SUPPORT
        		qm_mutex_unlock(&g_mem_lock);
			#endif

			}
		}
	}
}
/*-----------------------------------------------------------*/

uint32_t qm_free_mem_get( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

uint32_t qm_GetMinimumEverFreeqmMemSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/


void qm_heap_init( uint8_t* heap, uint32_t heap_len)
{
	uint8_t *pucAlignedHeap;
	uint32_t ulAddress;
	BlockLink_t *pxFirstFreeBlock;
	uint32_t xTotalHeapSize = heap_len;

	if(heap == NULL || heap_len == 0){
		return;
	}		

	/* Ensure the heap starts on a correctly aligned boundary. */
	ulAddress = ( uint32_t ) heap;

	if( ( ulAddress & CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK ) != 0 )
	{
		ulAddress += ( CONFIG_QM_MEM_BYTE_ALIGNMENT - 1 );
		ulAddress &= ~CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK;
		xTotalHeapSize -= ulAddress - ( uint32_t ) heap;
	}

	pucAlignedHeap = ( uint8_t * ) ulAddress;

	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	xStart.xBlockSize = ( uint32_t ) 0;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	ulAddress = ( ( uint32_t ) pucAlignedHeap ) + xTotalHeapSize;
	ulAddress -= xHeapStructSize;
	ulAddress &= ~CONFIG_QM_MEM_BYTE_ALIGNMENT_MASK;
	pxEnd = ( void * ) ulAddress;
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = ulAddress - ( uint32_t ) pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	/* Only one block exists - and it covers the entire usable heap space. */
	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

	/* Work out the position of the top bit in a uint32_t variable. */
	xBlockAllocatedBit = ( ( uint32_t ) 1 ) << ( ( sizeof( uint32_t ) * CONFIG_QM_MEM_BITS_PER_BYTE ) - 1 );

#if CONFIG_QM_OS_SUPPORT
 	qm_mutex_new(&g_mem_lock);
#endif
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
	uint8_t *puc;
	BlockLink_t *pxIterator;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}

}

void *qm_calloc(uint32_t num, uint32_t size)
{
	void* p=NULL;
	if(num == 0 || size == 0){
		return NULL;
	}

    p = qm_malloc(num*size);
	if(p != NULL){
		memset(p, 0, num*size);	
	}

	return p;
}  

#endif
