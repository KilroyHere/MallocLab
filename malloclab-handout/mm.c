/*
 * mm.c -  Simple allocator based on implicit free lists,
 *         first fit placement, and boundary tag coalescing.
 *
 * Each block has header and footer of the form:
 *
 *      63       32   31        1   0
 *      --------------------------------
 *     |   unused   | block_size | a/f |
 *      --------------------------------
 *
 * a/f is 1 iff the block is allocated. The list has the following form:
 *
 * begin                                       end
 * heap                                       heap
 *  ----------------------------------------------
 * | hdr(8:a) | zero or more usr blks | hdr(0:a) |
 *  ----------------------------------------------
 * | prologue |                       | epilogue |
 * | block    |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */
#include "memlib.h"
#include "mm.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Your info */
team_t team = {
    /* First and last name */
    "Aryan Patel",
    /* UID */
    "005329756",
    /* Custom message (16 chars) */
    "1 mor day pls",
};

typedef struct {
    uint32_t allocated : 1;
    uint32_t block_size : 31;
    uint32_t _;
} header_t;

typedef header_t footer_t;

typedef struct block_t {
    uint32_t allocated : 1;
    uint32_t block_size : 31;
    uint32_t _;
    union {
        struct {
            struct block_t* next;
            struct block_t* prev;
        };
        int payload[0]; 
    } body;
} block_t;

/* This enum can be used to set the allocated bit in the block */
enum block_state { FREE,
                   ALLOC };

#define CHUNKSIZE (1 << 16) /* initial heap size (bytes) *//////2^16
#define OVERHEAD (sizeof(header_t) + sizeof(footer_t)) /* overhead of the header and footer of an allocated block */
#define MIN_BLOCK_SIZE (32) /* the minimum block size needed to keep in a freelist (header + footer + next pointer + prev pointer) */
//Minimum Block size is 8 Bytes
#define LOG2(i)   31 - __builtin_clz(i)

/* Global variables */
static block_t *prologue; /* pointer to first block */
static block_t** segList;
static int LISTNUM = 11;

/* function prototypes for internal helper routines */


static block_t *extend_heap(size_t words,bool coal);
static void place(block_t *block, size_t asize);
static block_t *find_fit(size_t asize);
static block_t *coalesce(block_t *block);
static footer_t *get_footer(block_t *block);
static void printblock(block_t *block);
static void checkblock(block_t *block);
static void insertblock(block_t *block, int index);
static void deleteblock(block_t *block, int index);
static int listIndex(int x);

/*
 * mm_init - Initialize the memory manager
 */
/* $begin mminit */
int mm_init(void) 
{

    /*Initialize SegList */
    segList = mem_sbrk(8*LISTNUM);
    segList[0] = NULL;
    segList[1] = NULL;
    segList[2] = NULL;
    segList[3] = NULL;
    segList[4] = NULL;
    segList[5] = NULL;
    segList[6] = NULL;
    segList[7] = NULL;
    segList[8] = NULL;
    segList[9] = NULL;
    segList[10] = NULL;

    /* create the initial empty heap */
    if ((prologue = mem_sbrk(CHUNKSIZE)) == (void*)-1)
        return -1;
    /* initialize the prologue */
    prologue->allocated = ALLOC;
    prologue->block_size = sizeof(header_t);

    /* initialize the first free block */
    block_t *init_block = (void *)prologue + sizeof(header_t);
    init_block->allocated = FREE;
    init_block->block_size = CHUNKSIZE - OVERHEAD;
    footer_t *init_footer = get_footer(init_block);
    init_footer->allocated = FREE;
    init_footer->block_size = init_block->block_size;
   
    /* Updating the new block pointers */
    init_block->body.prev = NULL;
    init_block->body.next = NULL;
       
    //Setting up segList[10], with this huge block //
    segList[10] = init_block;

    /* initialize the epilogue - block size 0 will be used as a terminating condition */
    block_t *epilogue = (void *)init_block + init_block->block_size;
    epilogue->allocated = ALLOC;
    epilogue->block_size = 0;

    return 0;
}
/* $end mminit */



/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size) {
    uint32_t asize;       /* adjusted block size */
    uint32_t extendsize;  /* amount to extend heap if no fit */
    uint32_t extendwords; /* number of words to extend heap if no fit */

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    size += OVERHEAD;

    asize = ((size + 7) >> 3) << 3; /* align to multiple of 8 */

    if (asize < MIN_BLOCK_SIZE)
     {
        asize = MIN_BLOCK_SIZE;
     }
    block_t *block;


    //Small Blocks directly extend heap without coalesce
    if(asize <= 64 )
    {
        extendsize = asize;
        extendwords = extendsize >> 3;

        if ((block = extend_heap(extendwords,false)) != NULL) 
        {
            place(block,asize);
            return block->body.payload;
        }
    }

    /* Search the free list for a fit *///If valid, place
    block = find_fit(asize);
    if (block != NULL) 
    {
       // printheaplist();
        place(block, asize);
        return block->body.payload;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = (asize > CHUNKSIZE) ? asize : CHUNKSIZE; 
    extendwords = extendsize >> 3; // extendsize/8
    if ((block = extend_heap(extendwords,true)) != NULL) 
    {
        place(block, asize);
        return block->body.payload;
    }
    /* no more memory :( */
    return NULL;
}
/* $end mmmalloc */


/*
 * mm_free - Free a block
 */
/* $begin mmfree */
void mm_free(void *payload) {
    block_t *block = payload - sizeof(header_t);
    block->allocated = FREE;
    footer_t *footer = get_footer(block);
    footer->allocated = FREE;

    int index = listIndex(block->block_size);
    insertblock(block,index);
    coalesce(block);

}/* $end mmfree */


/*
 * mm_checkheap - Check the heap for consistency
 */ 
void mm_checkheap(int verbose) {
    block_t *block = prologue;

    if (verbose)
        printf("Heap (%p):\n", prologue);

    if (block->block_size != sizeof(header_t) || !block->allocated)
        printf("Bad prologue header\n");
    checkblock(prologue);

    /* iterate through the heap (both free and allocated blocks will be present) */
    for (block = (void*)prologue+prologue->block_size; block->block_size > 0; block = (void *)block + block->block_size) {
        if (verbose)
            printblock(block);
        checkblock(block);
    }

    if (verbose)
        printblock(block);
    if (block->block_size != 0 || !block->allocated)
        printf("Bad epilogue header\n");
}   /* $end mmcheckheap */


/////////////////////// N O T  - R E Q U I R E D ///////////////
/*
 * mm_realloc - naive implementation of mm_realloc
 * NO NEED TO CHANGE THIS CODE!
 */
void *mm_realloc(void *ptr, size_t size) {
    void *newp;
    size_t copySize;

    if ((newp = mm_malloc(size)) == NULL) {
        printf("ERROR: mm_malloc failed in mm_realloc\n");
        exit(1);
    }
    block_t* block = ptr - sizeof(header_t);
    copySize = block->block_size;
    if (size < copySize)
        copySize = size;
    memcpy(newp, ptr, copySize);
    mm_free(ptr);
    return newp; 
}
////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* The remaining routines are internal helper routines */////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int listIndex(int x)
{
    int num = LOG2(x) - 5;
    return ( num < 10 ? num : 10 );
}


static void insertblock(block_t *block, int index)
{
    if(segList[index] == NULL)
    {
        block->body.next = NULL;
        block->body.prev = NULL;
        segList[index] = block;
    }
    else
    {
        block->body.next = segList[index];
        block->body.prev = NULL;
        segList[index]->body.prev = block;
        segList[index] = block;
    }
}

static void deleteblock(block_t *block, int index)
{
    block_t* head_block = segList[index];
    if(head_block->body.prev == NULL && head_block->body.next == NULL)
        {
            segList[index] = NULL;
        }
        else
        {
            if(block == head_block)
            {
                block->body.next->body.prev = NULL;
                segList[index]= block->body.next;
            }
            else if (block->body.next == NULL)
            {
                block->body.prev->body.next = NULL;
            }
            else
            {
                block->body.prev->body.next = block->body.next;
                block->body.next->body.prev = block->body.prev; 
            }
        }
}




/*///////////////////////////////////////////////////////////////////////
 * extend_heap - Extend heap with free block and return its block pointer
 *///////////////////////////////////////////////////////////////////////
/* $begin mmextendheap */
static block_t *extend_heap(size_t words, bool coal) {
    block_t *block;
    uint32_t size;
    size = words << 3; // words*8
    if (size == 0 || (block = mem_sbrk(size)) == (void *)-1)
        return NULL;
    /* The newly acquired region will start directly after the epilogue block */
    /* Initialize free block header/footer and the new epilogue header */
    /* use old epilogue as new free block header */
    block = (void *)block - sizeof(header_t);
    block->allocated = FREE;
    block->block_size = size;
    /* free block footer */
    footer_t *block_footer = get_footer(block);
    block_footer->allocated = FREE;
    block_footer->block_size = block->block_size;

    ///Inserting this new block
    int index = listIndex(block->block_size);
    insertblock(block,index);

    /* new epilogue header */
    header_t *new_epilogue = (void *)block_footer + sizeof(header_t);
    new_epilogue->allocated = ALLOC;
    new_epilogue->block_size = 0;
    /* Coalesce if the previous block was free */
    if(coal)
        return coalesce(block);
    else
        return block;

}/* $end mmextendheap */
/////////////////////////////////////////////////////////////////////////////
 
 
/*///////////////////////////////////////////////////////////////////////
 * place - Place block of asize bytes at start of free block block
 *         and split if remainder would be at least minimum block size
 *////////////////////////////////////////////////////////////////////////
/* $begin mmplace */
static void place(block_t *block, size_t asize) {
    size_t split_size = block->block_size - asize;
    
    if (split_size >= MIN_BLOCK_SIZE) 
    {
        int index = listIndex(block->block_size);
        //Old Block deleted:
        deleteblock(block,index);

        /* split the block by updating the header and marking it allocated*/
        block->block_size = asize;
        block->allocated = ALLOC;
        /* set footer of allocated block*/
        footer_t *footer = get_footer(block);
        footer->block_size = asize;
        footer->allocated = ALLOC;
   

        /* update the header of the new free block */
        block_t *new_block = (void *)block + block->block_size;
        new_block->block_size = split_size;
        new_block->allocated = FREE;
        /* update the footer of the new free block */
        footer_t *new_footer = get_footer(new_block);
        new_footer->block_size = split_size;
        new_footer->allocated = FREE;

        index = listIndex(new_block->block_size);
        //Inserting the new block
        insertblock(new_block,index);

    } 
    else 
    {
         int index = listIndex(block->block_size);
         //Old Block deleted:
       deleteblock(block,index);
        /* splitting the block will cause a splinter so we just include it in the allocated block */
        block->allocated = ALLOC;
        footer_t *footer = get_footer(block);
        footer->allocated = ALLOC;
      
    }
}   /* $end mmplace */
///////////////////////////////////////////////////////////////////////


/*///////////////////////////////////////////////////////////////////////////////////////////////////////
 * find_fit - Find a fit for a block with asize bytes
 *//////////////////////////////////////////////////////////////////////////////////////////////////////

static block_t *find_fit(size_t asize) {
    block_t *b;
    int index = listIndex(asize);

    while(index <= 10)
    {
        b = segList[index] ;
        while(b != NULL)
        {   
        // block must be free and the size must be large enough to hold the request 
         if (!b->allocated && asize <= b->block_size)
            {
               return b;
            }
         b = b->body.next;
        }
        index++;
    }
    return NULL; // no fit 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////
/*/////////////////////////////////////////////////////////////////////
 * coalesce - boundary tag coalescing. Return ptr to coalesced block
 */////////////////////////////////////////////////////////////////////
 //////////////////////////////////////////////////////////////////////
static block_t *coalesce(block_t *block) {
    footer_t *prev_footer = (void *)block - sizeof(header_t);
    header_t *next_header = (void *)block + block->block_size;
    block_t *next_block = (void *)next_header;
    block_t *prev_block = (void *)prev_footer - prev_footer->block_size + sizeof(header_t);
    bool prev_alloc = prev_footer->allocated;
    bool next_alloc = next_header->allocated;

    if (prev_alloc && next_alloc) { /* Case 1 */
        /* no coalesceing */
        return block;
    }

    else if (prev_alloc && !next_alloc)
     {
        int index = listIndex(block->block_size);
        deleteblock(block,index);
        index = listIndex(next_block->block_size);
        deleteblock(next_block,index);

         /* Case 2 */
        /* Update header of current block to include next block's size */
        block->block_size += next_header->block_size;
        /* Update footer of next block to reflect new size */
        footer_t *next_footer = get_footer(block);
        next_footer->block_size = block->block_size;

        index = listIndex(block->block_size);
        insertblock(block,index);

    }

    else if (!prev_alloc && next_alloc) { 
        
        int index = listIndex(block->block_size);
        deleteblock(block,index);
        index = listIndex(prev_block->block_size);
        deleteblock(prev_block,index);

        /* Case 3 */
        /* Update header of prev block to include current block's size */
        
        prev_block->block_size += block->block_size;
        /* Update footer of current block to reflect new size */
        footer_t *footer = get_footer(prev_block);
        footer->block_size = prev_block->block_size;

        index = listIndex(prev_block->block_size);
        insertblock(prev_block,index);
        block = prev_block;
    }

    else { 
        
        int index = listIndex(block->block_size);
        deleteblock(block,index);
        index = listIndex(next_block->block_size);
        deleteblock(next_block,index);
        index = listIndex(prev_block->block_size);
        deleteblock(prev_block,index);
        
        /* Case 4 */
        /* Update header of prev block to include current and next block's size */
        block_t *prev_block = (void *)prev_footer - prev_footer->block_size + sizeof(header_t);
        prev_block->block_size += block->block_size + next_header->block_size;
        /* Update footer of next block to reflect new size */
        footer_t *next_footer = get_footer(prev_block);
        next_footer->block_size = prev_block->block_size;
        
        index = listIndex(prev_block->block_size);
        insertblock(prev_block,index);

        block = prev_block;
    }

    return block;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


static footer_t* get_footer(block_t *block) {
    return (void*)block + block->block_size - sizeof(footer_t);
}

static void printblock(block_t *block) {
    uint32_t hsize, halloc, fsize, falloc;

    hsize = block->block_size;
    halloc = block->allocated;
    footer_t *footer = get_footer(block);
    fsize = footer->block_size;
    falloc = footer->allocated;

    if (hsize == 0) {
        printf("%p: EOL\n", block);
        return;
    }

    printf("%p: header: [%d:%c] footer: [%d:%c]\n", block, hsize,
           (halloc ? 'a' : 'f'), fsize, (falloc ? 'a' : 'f'));
    
    if (block->allocated)
        printf("Allocated \n");
    else
    {
    if (block->body.prev == NULL)
        printf("NUL\n");
    else
        printf("%p \n",block->body.prev);
   
   
    if (block->body.next == NULL)
        printf("NUL\n");
    else
        printf("%p \n",block->body.next);
    }
}



static void checkblock(block_t *block) {
    if ((uint64_t)block->body.payload % 8) {
        printf("Error: payload for block at %p is not aligned\n", block);
    }
    footer_t *footer = get_footer(block);
    if (block->block_size != footer->block_size) {
        printf("Error: header does not match footer\n");
    }
}

