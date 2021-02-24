# MallocLab (Dynamic Memory Allocator)
My Implementation of malloc() and free() functions found in the Standard C Library. 

Dynamic memory allocator: 64-bit clean allocator based on SEGREGATED free lists, LIFO free block ordering, FIRST FIT placement, and boundary tag coalescing.
Blocks must be aligned to doubleword (8 byte) boundaries. Minimum block size is 32 bytes.

```
/*
 * mm.c -   My version of malloc is based on segregated free lists .
 The free lists are organized in the first CHUNKSIZE(2^16) bytes of the actual heap.
 The seglist essentially acts like a Hash table and is in ascending order of capacity to store free blocks (This encourages a best fit search) 
 When a block is allocated if the padding is more than 2 * DSIZE then it is split and added to segregated free list . 
 While mallocing , first the search is done through the segregated free list and then if nothing is found malloc is called . 
 
 Free blocks list: Implemented using segregated lists.The free blocks 
 are arranged in many linked lists that only contain blocks less than 
 fixed sizes.
 Headers: Both allocated and free blocks have headers that indicate
 the size of the blocks, current block's allocation status, and 
 previous block's allocation status.
 Footers: Used only for coalescing of free blocks. Footers are 
 identical to headers. They are included at the end of each FREE BLOCK
 for possible coalescing. Also, pointers to the previous(predecessor)
 and next(successor) free block in the segregated list are included 
 in each free block's payload.
 * Each block is of the form:
 * Free Blocks :     [ HDR | NEXTP | PREVP | FTR ] 
 * Bytes:              8      8        8      8   ==> MIN_BLK_SIZE = 32 bytes
 * 
 * Allocated Blocks: [ HDR |   PAYLOADS   | FTR ]
 *                         
 * Each block has header and footer of the form:
 * 8 Bytes:
 *         4 Bytes         4 Bytes
 *      --------------------------------
 *     |   unused   | block_size | a/f |
 *      --------------------------------
 *        
 * a/f is the allocation flag and is 1 iff the block is allocated. 
 *
 * Each segragated free list has the following form:
 * begin                                       end
 * heap                                       heap
 *  ----------------------------------------------
 * | hdr(8:a) | zero or more free blocks | hdr(0:a) |
 *  ----------------------------------------------
 * | prologue |                       | epilogue |
 * | block    |                       | block    |
 *
 * The allocated prologue and epilogue blocks are overhead that
 * eliminate edge conditions during coalescing.
 */
```
