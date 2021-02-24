# MallocLab
My Implementation of malloc() and free() functions found in the Standard C Library. 

Dynamic memory allocator: 64-bit clean allocator based on SEGREGATED free lists, LIFO free block ordering, FIRST FIT placement, and boundary tag coalescing.
Blocks must be aligned to doubleword (8 byte) boundaries. Minimum block size is 24 bytes.
