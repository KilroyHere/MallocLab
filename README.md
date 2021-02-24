# MallocLab
My Implementation of malloc() and free() functions found in the Standard C Library. 

Dynamic memory allocator: 64-bit clean allocator based on SEGREGATED free lists, LIFO free block ordering, FIRST FIT placement, and boundary tag coalescing.
Blocks must be aligned to doubleword (8 byte) boundaries. Minimum block size is 32 bytes.

```
/*
  Each Block  consists of a header and footer (8 bytes each) that stores overhead info about the block:
  
  Free Blocks [ HDR | NEXTP | PREVP | FTR ] 
  Bytes:         8      8        8      8   ==> MIN_BLK_SIZE = 32 bytes
  Allocated BlocksA [ HDR |    PAYLOADS   | FTR ]
*/
```
