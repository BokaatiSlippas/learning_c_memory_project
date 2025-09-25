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
    if (total_size<MIN_MMAP_SIZE) {
        total_size=MIN_MMAP_SIZE;
    }
    void *memory = mmap(
        NULL, // can choose from available addresses
        total_size,
        PROT_READ | PROT_WRITE, // read or write
        MAP_PRIVATE | MAP_ANONYMOUS, // changes are private to the process -> CoW on fork AND pure RAM so disappears when process exits as its not file-backed
        -1, // fd and offset ignored because anonymous
        0
    );
    if (memory==MAP_FAILED) {
        return NULL;
    }
    block_header *header = (block_header*)memory;
    header->size = total_size-HEADER_SIZE;
    header->free = 1;
    header->next = NULL;
    header->prev = NULL;
    return header;
}


void *malloc(size_t size) {
    if (!size) return NULL;
    size_t aligned_size = ALIGN(size); // make sure sizes of 8
    // first-fit cuz easiest hehe
    block_header *block = head;
    block_header *prev = head;
    while (block) {
        if (block->free==1 && block->size>=aligned_size) {
            block->free=0;
            if (block->size > aligned_size+HEADER_SIZE+ALIGNMENT)
            split_block(block, size);
            return (void*)(block+1);
        }
        prev=block;
        block=block->next;
    }
    // No free block found request more
    block_header *new_block = get_memory_from_os(aligned_size);
    if (!new_block) return NULL;
    new_block->free=0;
    new_block->size = aligned_size;
    if (!head) {
        head=new_block;
    } else {
        prev->next=new_block;
        new_block->prev=prev;
        tail=new_block;
    }
    return (void*)(new_block+1);
}

void free(void *ptr) {
    if (!ptr) return;
    block_header *block = (block_header*)ptr-1;
    block->free = 1;
    if (block->next && block->next->free)
}
