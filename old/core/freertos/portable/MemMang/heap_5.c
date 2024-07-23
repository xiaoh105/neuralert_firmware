/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of https://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 *  { ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 *  { ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 *  { NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include <stdlib.h>
#include <string.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "common_def.h"
#include "da16200_arch.h"  // ATTRIBUTE_RAM_FUNC

//#define	SAVE_TASK_NAME_SIZE		configMAX_TASK_NAME_LEN
#define	SAVE_TASK_NAME_SIZE			16

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
    char				blockOwner[SAVE_TASK_NAME_SIZE];

} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
 * block must by correctly byte aligned. */
static const size_t xHeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, * pxEnd = NULL;

/* Keeps track of the number of calls to allocate and free memory as well as the
 * number of free bytes remaining, but says nothing about fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
 * member of an BlockLink_t structure is set then the block belongs to the
 * application.  When the bit is free the block is still part of the free heap
 * space. */
static size_t xBlockAllocatedBit = 0;

#ifdef	USED_HEAP_BLOCK_STATUS
//////////////////////////////////////////////////////////////////////////////
/********** heap_req queue structure **********/
#define MAX_HEAP_REQ_Q		500	/* <-- 400 */

typedef struct HEAP_USE_TABLE {
	char	taskName[SAVE_TASK_NAME_SIZE];
	size_t		useSize;
} HEAP_USE_TABLE_T;

struct HEAP_REQ_QUEUE_NODE {
	size_t 							size;
	BlockLink_t						*heapReqData;
	struct	HEAP_REQ_QUEUE_NODE 	*used_next;
	struct	HEAP_REQ_QUEUE_NODE 	*next;
};

struct FREE_HEAP_REQ_QUEUE {
	unsigned char	space[sizeof(struct HEAP_REQ_QUEUE_NODE)-8];
	struct	FREE_HEAP_REQ_QUEUE	*used_next;
	struct	FREE_HEAP_REQ_QUEUE	*next;
};

struct FREE_HEAP_REQ_QUEUE_LIST {
	struct FREE_HEAP_REQ_QUEUE *point;
	unsigned short remain;
};

/********** node link structure **********/
#define HEAP_LINK	0
#define XXXX_LINK	1
#define YYYY_LINK	2

//#define	MAX_HEAP_REQ_NODE_CNT	3	/* 0:HEAP, 1:XXXX, 2:YYYY */
#define	MAX_HEAP_REQ_NODE_CNT		1	/* 0:HEAP, 1:XXXX, 2:YYYY */

typedef struct heapReq_link {
	unsigned int	HEAP_REQ_Cnt[MAX_HEAP_REQ_NODE_CNT];
	unsigned int	maxAddCount[MAX_HEAP_REQ_NODE_CNT];
	struct	HEAP_REQ_QUEUE_NODE	*HEAP_REQ_Node[MAX_HEAP_REQ_NODE_CNT];
} HEAP_REQ_LINK;

/*
 * Global functions
 */
void Init_HEAP_REQ_q(unsigned int statisticsClearFlag);
int get_HEAP_REQ_queue(void);
int free_HEAP_REQ_queue(struct FREE_HEAP_REQ_QUEUE *cur);
void HEAP_REQ_q_info(void);
void HEAP_REQ_Linker_info(void);
struct HEAP_REQ_QUEUE_NODE *add_HEAP_REQ_Node(unsigned int index, struct HEAP_REQ_QUEUE_NODE* queue);
struct HEAP_REQ_QUEUE_NODE *get_HEAP_REQ_Node(BlockLink_t *pxLink);
void HEAP_REQ_LinkerInit(void);
void heapReqQueueInit(unsigned int statisticsClear);

struct FREE_HEAP_REQ_QUEUE_LIST free_HEAP_REQ_q_list;
unsigned long HEAP_REQ_queue_start;
unsigned long HEAP_REQ_queue_end;
unsigned HEAP_REQ_queue_cnt;
unsigned HEAP_REQ_queue_full_cnt;
unsigned HEAP_REQ_queue_use_cnt;
unsigned HEAP_REQ_queue_free_fail_cnt;
unsigned get_HEAP_REQ_queue_cnt;
unsigned free_HEAP_REQ_queue_cnt;
struct HEAP_REQ_QUEUE_NODE HEAP_REQ_Q_BUFFER[MAX_HEAP_REQ_Q];

struct FREE_HEAP_REQ_QUEUE_LIST used_HEAP_REQ_q_list;
unsigned used_HEAP_REQ_queue_cnt;

int get_HEAP_REQ_queue(void);
extern void HEAP_REQ_LinkerInit(void);

HEAP_REQ_LINK 		HEAP_REQ_Link;
HEAP_USE_TABLE_T	use_table[30];

//////////////////////////////////////////////////////////////////////////////
#endif	/* USED_HEAP_BLOCK_STATUS */

/*-----------------------------------------------------------*/
ATTRIBUTE_RAM_FUNC
void * pvPortMalloc( size_t xWantedSize )
{
    BlockLink_t * pxBlock = NULL, * pxPreviousBlock, * pxNewBlockLink;
    void * pvReturn = NULL;
#ifdef	USED_HEAP_BLOCK_STATUS
struct HEAP_REQ_QUEUE_NODE *queue = NULL;
#endif	/* USED_HEAP_BLOCK_STATUS */

    /* The heap must be initialised before the first call to
     * prvPortMalloc(). */
    configASSERT( pxEnd );

    vTaskSuspendAll();
    {
        /* Check the requested block size is not so large that the top bit is
         * set.  The top bit of the block size member of the BlockLink_t structure
         * is used to determine who owns the block - the application or the
         * kernel, so it must be free. */
        if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
        {
            /* The wanted size is increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. */
            if( ( xWantedSize > 0 ) && 
                ( ( xWantedSize + xHeapStructSize ) >  xWantedSize ) ) /* Overflow check */
            {
                xWantedSize += xHeapStructSize;

                /* Ensure that blocks are always aligned */
                if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
                {
                    /* Byte alignment required. Check for overflow */
                    if( ( xWantedSize + ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) ) ) >
                         xWantedSize )
                    {
                        xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
                    } 
                    else 
                    {
                        xWantedSize = 0;
                    }
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                xWantedSize = 0;
            }

            if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
            {
                /* Traverse the list from the start	(lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if( pxBlock != pxEnd )
                {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                     * two. */
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;
#ifdef	USED_HEAP_BLOCK_STATUS
                        extern char	*xTaskGetCurrentTaskName( void );

						memset(pxBlock->blockOwner, 0, SAVE_TASK_NAME_SIZE);
						strncpy(pxBlock->blockOwner, xTaskGetCurrentTaskName(), SAVE_TASK_NAME_SIZE-1);

						queue = (struct HEAP_REQ_QUEUE_NODE *)get_HEAP_REQ_queue();
						if (!queue) {
#ifdef DEBUG_QUEUE_STATUS
							PRINTF(" [%s] get queue error !!!\n", __func__);  /* TEMPORARY */
#endif
						}
						else {
							queue->heapReqData = pxBlock;
							queue->size = xWantedSize;
							add_HEAP_REQ_Node(HEAP_LINK, queue);
						}
#endif	/* USED_HEAP_BLOCK_STATUS */

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
                    {
                        xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    pxBlock->xBlockSize |= xBlockAllocatedBit;
                    pxBlock->pxNextFreeBlock = NULL;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xWantedSize );
    }
    ( void ) xTaskResumeAll();

    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
        {
            if( pvReturn == NULL )
            {
                extern void vApplicationMallocFailedHook( void );
                vApplicationMallocFailedHook();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    #endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    return pvReturn;
}
/*-----------------------------------------------------------*/
ATTRIBUTE_RAM_FUNC
void vPortFree( void * pv )
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;
#ifdef	USED_HEAP_BLOCK_STATUS
struct HEAP_REQ_QUEUE_NODE *queue = NULL;
#endif	/* USED_HEAP_BLOCK_STATUS */

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        /* Check the block is actually allocated. */
        configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /* The block is being returned to the heap - it is no longer
                 * allocated. */
                pxLink->xBlockSize &= ~xBlockAllocatedBit;

                vTaskSuspendAll();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
#ifdef	USED_HEAP_BLOCK_STATUS
					queue = get_HEAP_REQ_Node(pxLink);
					if (queue) {
						free_HEAP_REQ_queue((struct FREE_HEAP_REQ_QUEUE *)queue);
					}
					else {
#ifdef DEBUG_QUEUE_STATUS
						PRINTF(RED_COLOR " Not found queue(0x%8x) \r\n" CLEAR_COLOR, pxLink);
#endif
					}
#endif	/* USED_HEAP_BLOCK_STATUS */
                    prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
                }
                ( void ) xTaskResumeAll();
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/
ATTRIBUTE_RAM_FUNC
size_t xPortGetFreeHeapSize( void )
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/
ATTRIBUTE_RAM_FUNC
size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/
ATTRIBUTE_RAM_FUNC
static void prvInsertBlockIntoFreeList( BlockLink_t * pxBlockToInsert )
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxIterator;

    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
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
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
{
    BlockLink_t * pxFirstFreeBlockInRegion = NULL, * pxPreviousFreeBlock;
    size_t xAlignedHeap;
    size_t xTotalRegionSize, xTotalHeapSize = 0;
    BaseType_t xDefinedRegions = 0;
    size_t xAddress;
    const HeapRegion_t * pxHeapRegion;

    /* Can only call once! */
    configASSERT( pxEnd == NULL );

    pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

    while( pxHeapRegion->xSizeInBytes > 0 )
    {
        xTotalRegionSize = pxHeapRegion->xSizeInBytes;

        /* Ensure the heap region starts on a correctly aligned boundary. */
        xAddress = ( size_t ) pxHeapRegion->pucStartAddress;

        if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
        {
            xAddress += ( portBYTE_ALIGNMENT - 1 );
            xAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

            /* Adjust the size for the bytes lost to alignment. */
            xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
        }

        xAlignedHeap = xAddress;

        /* Set xStart if it has not already been set. */
        if( xDefinedRegions == 0 )
        {
            /* xStart is used to hold a pointer to the first item in the list of
             *  free blocks.  The void cast is used to prevent compiler warnings. */
            xStart.pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
            xStart.xBlockSize = ( size_t ) 0;
        }
        else
        {
            /* Should only get here if one region has already been added to the
             * heap. */
            configASSERT( pxEnd != NULL );

            /* Check blocks are passed in with increasing start addresses. */
            configASSERT( xAddress > ( size_t ) pxEnd );
        }

        /* Remember the location of the end marker in the previous region, if
         * any. */
        pxPreviousFreeBlock = pxEnd;

        /* pxEnd is used to mark the end of the list of free blocks and is
         * inserted at the end of the region space. */
        xAddress = xAlignedHeap + xTotalRegionSize;
        xAddress -= xHeapStructSize;
        xAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
        pxEnd = ( BlockLink_t * ) xAddress;
        pxEnd->xBlockSize = 0;
        pxEnd->pxNextFreeBlock = NULL;

        /* To start with there is a single free block in this region that is
         * sized to take up the entire heap region minus the space taken by the
         * free block structure. */
        pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
        pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
        pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

        /* If this is not the first region that makes up the entire heap space
         * then link the previous region to this region. */
        if( pxPreviousFreeBlock != NULL )
        {
            pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
        }

        xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

        /* Move onto the next HeapRegion_t structure. */
        xDefinedRegions++;
        pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
    }

    xMinimumEverFreeBytesRemaining = xTotalHeapSize;
    xFreeBytesRemaining = xTotalHeapSize;

    /* Check something was actually defined before it is accessed. */
    configASSERT( xTotalHeapSize );

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );

#ifdef	USED_HEAP_BLOCK_STATUS
	heapReqQueueInit(1);
#endif	/* USED_HEAP_BLOCK_STATUS */
}

#ifdef	USED_HEAP_BLOCK_STATUS
void printUsedHeapBlockInfo()
{
	unsigned int	Index = 0;
	int				cnt	= 0;
	int				useTableIndex = 0;
	size_t			totalUseSize = 0;
	struct HEAP_REQ_QUEUE_NODE	*curptr;
	struct HEAP_REQ_QUEUE_NODE	**heapReq_nodeptr;

	PRINTF("\r\n");
#ifdef DEBUG_QUEUE_STATUS
	HEAP_REQ_Linker_info();
	HEAP_REQ_q_info();
#endif

	heapReq_nodeptr = &HEAP_REQ_Link.HEAP_REQ_Node[0];

	if (heapReq_nodeptr[Index] == 0) {
		PRINTF(" No information \r\n");
		return;
	}
	else {
		curptr = heapReq_nodeptr[Index];
	}

	PRINTF(" == HEAP usage information by task == \r\n");
	memset(use_table, 0, sizeof(HEAP_USE_TABLE_T)*30);

	while(1) {

		if (curptr->heapReqData->xBlockSize & xBlockAllocatedBit) {
			++cnt;
#ifdef DEBUG_QUEUE_STATUS
			PRINTF(" %2d) %8s\t %6d(0x%x) \r\n",
					cnt, curptr->heapReqData->blockOwner, curptr->size, curptr->heapReqData->xBlockSize);
#endif
			for (useTableIndex = 0; useTableIndex < 30; ++useTableIndex) {
				if (use_table[useTableIndex].taskName[0] == 0) {
					strncpy(use_table[useTableIndex].taskName, curptr->heapReqData->blockOwner, SAVE_TASK_NAME_SIZE-1);
					use_table[useTableIndex].useSize += curptr->size;
					break;
				}
				else if (!strncmp(use_table[useTableIndex].taskName, curptr->heapReqData->blockOwner, SAVE_TASK_NAME_SIZE-1)){
					use_table[useTableIndex].useSize += curptr->size;
					break;
				}
			}

			if (useTableIndex == 30) {
				PRINTF(RED_COLOR " [%s] use_table overflow\n" CLEAR_COLOR, __func__);
			}
		}

		if (curptr->next == 0) {
			//return;
			break;
		}
		else {
			curptr = curptr->next;
		}
	}

	strncpy(use_table[0].taskName, "free_rtos", SAVE_TASK_NAME_SIZE-1);	/* maybe system task */

	PRINTF(CYAN_COLOR " Task\t\t\t   Size\n " CLEAR_COLOR);
	print_separate_bar('-', 30, 1);

	for (useTableIndex = 0; useTableIndex < 30; ++useTableIndex) {
		if (use_table[useTableIndex].taskName[0] != 0) {

			PRINTF(CYAN_COLOR "%3d) %-14s\t %6d\n" CLEAR_COLOR,
							useTableIndex+1,
							use_table[useTableIndex].taskName,
							use_table[useTableIndex].useSize);

			totalUseSize += use_table[useTableIndex].useSize;
		}
		else {
			break;
		}
	}

	PRINTF(" ");
	print_separate_bar('-', 30, 1);
	PRINTF(CYAN_COLOR " Total\t\t\t %6d\n\n" CLEAR_COLOR, totalUseSize);

#if 0	// FOR_DEBUGGING
    PRINTF(GREEN_COLOR " HEAP_REQ alloc count     :%4d\n" CLEAR_COLOR, HEAP_REQ_Link.HEAP_REQ_Cnt[HEAP_LINK]);
    PRINTF(GREEN_COLOR " HEAP_REQ alloc Max count :%4d\n" CLEAR_COLOR, HEAP_REQ_Link.maxAddCount[HEAP_LINK]);
#endif

	if (useTableIndex == 30) {
			PRINTF(RED_COLOR " [%s] print use_table overflow\n" CLEAR_COLOR, __func__);
	}

 	return;
}

//////////////////////////////////////////////////////////////////////////////
void Init_HEAP_REQ_q(unsigned int statisticsClearFlag)
{
	register int i;
	struct	HEAP_REQ_QUEUE_NODE *FREE_HEAP_REQ_Q;

	free_HEAP_REQ_q_list.point = 0;
	free_HEAP_REQ_q_list.remain = 0;

	FREE_HEAP_REQ_Q = &HEAP_REQ_Q_BUFFER[0];

	HEAP_REQ_queue_start = (unsigned long)FREE_HEAP_REQ_Q;
	free_HEAP_REQ_q_list.point = (struct FREE_HEAP_REQ_QUEUE *)FREE_HEAP_REQ_Q;
	free_HEAP_REQ_q_list.remain = MAX_HEAP_REQ_Q;
	for(i = 0; i < MAX_HEAP_REQ_Q - 1; i++) {
		FREE_HEAP_REQ_Q->next = FREE_HEAP_REQ_Q + 1;
		FREE_HEAP_REQ_Q++;
	}
	FREE_HEAP_REQ_Q->next = (struct HEAP_REQ_QUEUE_NODE *)NULL;

	HEAP_REQ_queue_end = (unsigned long)FREE_HEAP_REQ_Q;
	HEAP_REQ_queue_cnt = MAX_HEAP_REQ_Q;

	/* STATISTICS */
	if (statisticsClearFlag) {
		HEAP_REQ_queue_full_cnt = 0;
		HEAP_REQ_queue_use_cnt = 0;
		HEAP_REQ_queue_free_fail_cnt = 0;
		free_HEAP_REQ_queue_cnt = 0;
		get_HEAP_REQ_queue_cnt = 0;
	}

	used_HEAP_REQ_q_list.point = 0;
	used_HEAP_REQ_q_list.remain = 0;
	used_HEAP_REQ_queue_cnt = 0;
}

ATTRIBUTE_RAM_FUNC
int get_HEAP_REQ_queue(void)
{
	struct FREE_HEAP_REQ_QUEUE *cur;
	int	remain;

	remain = free_HEAP_REQ_q_list.remain;

	if (remain > 2) {
		free_HEAP_REQ_q_list.remain--;
		cur = free_HEAP_REQ_q_list.point;
		free_HEAP_REQ_q_list.point = cur->next;
		++HEAP_REQ_queue_use_cnt;
		++get_HEAP_REQ_queue_cnt;
	}
	else {
		++HEAP_REQ_queue_full_cnt;
#ifdef DEBUG_QUEUE_STATUS
		PRINTF(" [%s] Critical error Queue full \n", __func__); /* TEMPORARY */
#endif
		return(0);
	}

	cur->used_next = used_HEAP_REQ_q_list.point;	/* put in the head of the list */
	used_HEAP_REQ_q_list.point = cur;
	used_HEAP_REQ_q_list.remain++;

	return((int)cur);
}

ATTRIBUTE_RAM_FUNC
int free_HEAP_REQ_queue(struct FREE_HEAP_REQ_QUEUE *cur)
{
	struct FREE_HEAP_REQ_QUEUE *ptr;
	struct FREE_HEAP_REQ_QUEUE *pre_ptr;
	unsigned int	FindFlag = pdFALSE;

	if ((unsigned long)cur<HEAP_REQ_queue_start || (unsigned long)cur>HEAP_REQ_queue_end || ((
		(unsigned long)cur-HEAP_REQ_queue_start) % sizeof(struct HEAP_REQ_QUEUE_NODE))) {
		HEAP_REQ_queue_free_fail_cnt++;
#ifdef DEBUG_QUEUE_STATUS
		PRINTF(" [%s] Failed free queue - (%d) \r\n", __func__, HEAP_REQ_queue_free_fail_cnt);
#endif

		return(-1);
	}

	if (used_HEAP_REQ_q_list.point == cur) {	/* Find First Queue */
		FindFlag = pdTRUE;
		used_HEAP_REQ_q_list.remain--;
		if (used_HEAP_REQ_q_list.remain == 0) {
			used_HEAP_REQ_q_list.point = 0;
		}
		used_HEAP_REQ_q_list.point = cur->used_next;
	}
	else {
		ptr = used_HEAP_REQ_q_list.point;
		pre_ptr = ptr;

		while (ptr) {
			if (ptr == cur) {
				FindFlag = pdTRUE;
				used_HEAP_REQ_q_list.remain--;
				if (used_HEAP_REQ_q_list.remain == 0) {
					used_HEAP_REQ_q_list.point = 0;
					break;
				}
				pre_ptr->used_next = cur->used_next;
			}
			pre_ptr = ptr;
			ptr = ptr->used_next;
		}
	}

	if (FindFlag == pdFALSE) {
		/* ERROR */
	}

	cur->next = free_HEAP_REQ_q_list.point;
	free_HEAP_REQ_q_list.point = cur;
	free_HEAP_REQ_q_list.remain++;
	--HEAP_REQ_queue_use_cnt;
	++free_HEAP_REQ_queue_cnt;

	return(0);
}

void HEAP_REQ_q_info(void)
{
	PRINTF(" <HEAP_REQ_Q Buffer info> \r\n");
	PRINTF("  HEAP_REQ_Q total count    :%8d \r\n", HEAP_REQ_queue_cnt);
	PRINTF("  HEAP_REQ_Q remain count   :%8d \r\n", free_HEAP_REQ_q_list.remain);
	PRINTF("  HEAP_REQ_Q use count      :%8d \r\n", HEAP_REQ_queue_use_cnt);
	PRINTF("  get HEAP_REQ_Q count      :%8d \r\n", get_HEAP_REQ_queue_cnt);
	PRINTF("  free HEAP_REQ_Q count     :%8d \r\n", free_HEAP_REQ_queue_cnt);
	PRINTF("  HEAP_REQ_Q full count     :%8d \r\n", HEAP_REQ_queue_full_cnt);
	PRINTF("  HEAP_REQ_ree fail count:%8d \r\n", HEAP_REQ_queue_free_fail_cnt);
}

ATTRIBUTE_RAM_FUNC
struct HEAP_REQ_QUEUE_NODE *add_HEAP_REQ_Node(unsigned int index, struct HEAP_REQ_QUEUE_NODE* queue)
{
	unsigned int	Index;
	struct HEAP_REQ_QUEUE_NODE	**heapReq_nodeptr;
	struct HEAP_REQ_QUEUE_NODE	*curptr;
	struct HEAP_REQ_QUEUE_NODE	*preptr;

	heapReq_nodeptr = &HEAP_REQ_Link.HEAP_REQ_Node[0];
	Index = index;

	if (heapReq_nodeptr[Index] == 0) {	/* first node add */
		curptr = queue;
		if (curptr == 0) {
#ifdef DEBUG_QUEUE_STATUS
			PRINTF(" [%s] get_HEAP_REQ_queue Error \n", __func__);
#endif
			goto fail;
		}

		heapReq_nodeptr[Index] = curptr;	/* LINK */
		curptr->next = 0;

		++HEAP_REQ_Link.HEAP_REQ_Cnt[Index];
		if (HEAP_REQ_Link.maxAddCount[Index] < HEAP_REQ_Link.HEAP_REQ_Cnt[Index]) {
				HEAP_REQ_Link.maxAddCount[Index] = HEAP_REQ_Link.HEAP_REQ_Cnt[Index];
		}

#ifdef DEBUG_QUEUE_STATUS
		PRINTF(CYAN_COLOR " [%s] Block addr:0x%x(%d) cnt:%d \r\n" CLEAR_COLOR, __func__, curptr->heapReqData, curptr->size, HEAP_REQ_Link.HEAP_REQ_Cnt[Index]);
#endif
		return(curptr);
	}
	else {
		curptr = heapReq_nodeptr[Index];
	}

	preptr = curptr;
	while(curptr) {
		preptr = curptr;
		curptr = curptr->next;
	}

	preptr->next = queue;
	curptr = preptr->next;
	curptr->next = 0;

#ifdef DEBUG_QUEUE_STATUS
	PRINTF(CYAN_COLOR " [%s] Block addr:0x%x(%d) cnt:%d \r\n" CLEAR_COLOR, __func__, curptr->heapReqData, curptr->size, HEAP_REQ_Link.HEAP_REQ_Cnt[Index]);
#endif
	++HEAP_REQ_Link.HEAP_REQ_Cnt[Index];
	if (HEAP_REQ_Link.maxAddCount[Index] < HEAP_REQ_Link.HEAP_REQ_Cnt[Index]) {
			HEAP_REQ_Link.maxAddCount[Index] = HEAP_REQ_Link.HEAP_REQ_Cnt[Index];
	}

	return(curptr);

fail:
	return(0);
}

ATTRIBUTE_RAM_FUNC
struct HEAP_REQ_QUEUE_NODE *get_HEAP_REQ_Node(BlockLink_t *pxLink)
{
	struct HEAP_REQ_QUEUE_NODE	**heapReq_nodeptr;
	struct HEAP_REQ_QUEUE_NODE	*curptr;
	struct HEAP_REQ_QUEUE_NODE	*preptr;
	unsigned int	Index = HEAP_LINK;

	heapReq_nodeptr = &HEAP_REQ_Link.HEAP_REQ_Node[0];

	if (heapReq_nodeptr[Index] == 0) {
#ifdef DEBUG_QUEUE_STATUS
		PRINTF(YELLOW_COLOR " [%s] Not found 0x%8x (first) \r\n" CLEAR_COLOR, __func__, pxLink);
#endif
		return NULL;
	}
	else {
		curptr = heapReq_nodeptr[Index];
		preptr = curptr;
	}

	if (curptr->heapReqData == pxLink) {
		--HEAP_REQ_Link.HEAP_REQ_Cnt[Index];
		//heapReq_nodeptr[Index] = curptr->next;
		preptr->next = curptr->next;
		curptr->next = 0;
#ifdef DEBUG_QUEUE_STATUS
		PRINTF(YELLOW_COLOR " [%s] Block addr:0x%x(%d) cnt:%d \r\n" CLEAR_COLOR, __func__, curptr->heapReqData, curptr->size, HEAP_REQ_Link.HEAP_REQ_Cnt[Index]);
#endif
		return curptr;
	}

	while(1) {

		if (curptr->heapReqData == pxLink) {
			--HEAP_REQ_Link.HEAP_REQ_Cnt[Index];
			preptr->next = curptr->next;
#ifdef DEBUG_QUEUE_STATUS
			PRINTF(YELLOW_COLOR " [%s] Block addr:0x%x(%d) cnt:%d \r\n" CLEAR_COLOR, __func__, curptr->heapReqData, curptr->size, HEAP_REQ_Link.HEAP_REQ_Cnt[Index]);
#endif
			curptr->next = 0;
 			return curptr;
		}

		if (curptr->next == 0) {
#ifdef DEBUG_QUEUE_STATUS
			PRINTF(YELLOW_COLOR " [%s] Not found 0x%8x (final) \r\n" CLEAR_COLOR, __func__, pxLink);
#endif
			return NULL;
		}
		else {
			preptr = curptr;
			curptr = preptr->next;
		}
	}

#ifdef DEBUG_QUEUE_STATUS
	PRINTF(YELLOW_COLOR " [%s] Not found 0x%8x (end) \r\n" CLEAR_COLOR, __func__, pxLink);
#endif
 	return NULL;
}

void HEAP_REQ_LinkerInit(void)
{
	memset(&HEAP_REQ_Link, 0, sizeof(HEAP_REQ_Link));
}

void heapReqQueueInit(unsigned int statisticsClear)
{
	HEAP_REQ_LinkerInit();
	Init_HEAP_REQ_q(statisticsClear);
}

void HEAP_REQ_Linker_info(void)
{
	PRINTF(" <HEAP_REQ Linker info> \r\n");
	PRINTF("  HEAP_REQ Link count :%4d \r\n", HEAP_REQ_Link.HEAP_REQ_Cnt[HEAP_LINK]);
	PRINTF("  HEAP_REQ Link Max count :%4d \r\n", HEAP_REQ_Link.maxAddCount[HEAP_LINK]);
	//PRINTF("  HEAP_REQ A_TX Link count :%4d \r\n", HEAP_REQ_Link.HEAP_REQ_Cnt[XXXX_LINK]);
	//PRINTF("  HEAP_REQ A_RX Link count :%4d \r\n", HEAP_REQ_Link.HEAP_REQ_Cnt[YYYY_LINK]);
	PRINTF("\r\n");

}
#endif	/*USED_HEAP_BLOCK_STATUS*/

//////////////////////////////////////////////////////////////////////////////
