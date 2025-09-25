#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define ALIGNMENT 8 // force 8 byte chunks
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1)) // force 8 byte chunks
#define MIN_MMAP_SIZE (64 * 1024) // 64KB minimum mmap allocation

typedef struct block_header {
    size_t size;
    struct block_header *next;
    struct block_header *prev;
    int free;
} block_header; // We can call block_header instead of struct block_header

#define HEADER_SIZE ALIGN(sizeof(block_header))
static block_header *head = NULL;
static block_header *tail = NULL;


void split_block(block_header *block, size_t size) {
    size_t allocated = ALIGN(HEADER_SIZE+size);
    size_t remaining = block->size - allocated;
    if (remaining > ALIGN(HEADER_SIZE+ALIGNMENT)) { // any smaller remaining would be useless
        block_header *new_block = (block_header*)((char*)block + allocated); // we do pointer arithmetic to go to start of new_block location
        new_block->size = allocated-HEADER_SIZE;
        new_block->free = 1;
        new_block->prev = block;
        new_block->next = block->next;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        block->size = size;
        if (tail == block) {
            tail = new_block;
        }
    }
}

void *get_memory_from_os(size_t size) {
    size_t total_size = ALIGN(size+HEADER_SIZE);
}


void *malloc(size_t size) {
    if (!size) return NULL;
    size = ALIGN(size); // make sure sizes of 8
    // first-fit cuz easiest hehe
    block_header* block = head;
    while (block) {
        if (block->free==1 && block->size>=size) {
            block->free=0;
            split_block(block, size);
            return (void*)(block+1);
        }
        block=block->next;
    }
    // No free block found request more

}