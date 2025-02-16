#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16

typedef struct BlockHeader {
    struct BlockHeader *next;
} BlockHeader;

typedef struct Allocator {
    BlockHeader **free_lists;
    size_t num_lists;
    void *base_address;
    size_t total_size;
} Allocator;

static int calculate_log2(int value) {
    int result = -1;
    while (value > 0) {
        value >>= 1;
        result++;
    }
    return result;
}

Allocator* create_allocator(void *memory, size_t size) {
    if (memory == NULL || size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *)memory;
    allocator->base_address = memory;
    allocator->total_size = size;

    size_t min_usable_size = sizeof(BlockHeader) + MIN_BLOCK_SIZE;
    size_t max_block_size = (size < 32) ? 32 : size;

    allocator->num_lists = (size_t)(calculate_log2(max_block_size) / 2) + 3;
    allocator->free_lists = (BlockHeader **)((char *)memory + sizeof(Allocator));

    for (size_t i = 0; i < allocator->num_lists; i++) {
        allocator->free_lists[i] = NULL;
    }

    void *current_block = (char *)memory + sizeof(Allocator) + allocator->num_lists * sizeof(BlockHeader *);
    size_t remaining_size = size - sizeof(Allocator) - allocator->num_lists * sizeof(BlockHeader *);

    size_t block_size = MIN_BLOCK_SIZE;
    while (remaining_size >= min_usable_size) {
        if (block_size > remaining_size) {
            break;
        }

        size_t num_blocks = (remaining_size >= (block_size + sizeof(BlockHeader)) * 2) ? 2 : 1;

        for (size_t i = 0; i < num_blocks; i++) {
            BlockHeader *header = (BlockHeader *)current_block;
            size_t index = (block_size == 0) ? 0 : calculate_log2(block_size);
            header->next = allocator->free_lists[index];
            allocator->free_lists[index] = header;

            current_block = (char *)current_block + block_size;
            remaining_size -= block_size;
        }

        block_size <<= 1;
    }

    return allocator;
}

void destroy_allocator(Allocator *allocator) {
    if (allocator != NULL) {
        munmap(allocator->base_address, allocator->total_size);
    }
}

void* allocate_memory(Allocator *allocator, size_t size) {
    if (allocator == NULL || size == 0) {
        return NULL;
    }

    size_t index = (size == 0) ? 0 : calculate_log2(size) + 1;
    if (index >= allocator->num_lists) {
        index = allocator->num_lists - 1;
    }

    while (index < allocator->num_lists && allocator->free_lists[index] == NULL) {
        index++;
    }

    if (index >= allocator->num_lists) {
        return NULL;
    }

    BlockHeader *block = allocator->free_lists[index];
    allocator->free_lists[index] = block->next;

    return (void *)((char *)block + sizeof(BlockHeader));
}

void free_memory(Allocator *allocator, void *ptr) {
    if (allocator == NULL || ptr == NULL) {
        return;
    }

    BlockHeader *block = (BlockHeader *)((char *)ptr - sizeof(BlockHeader));
    size_t block_offset = (char *)block - (char *)allocator->base_address;

    size_t block_size = MIN_BLOCK_SIZE;
    while ((block_size << 1) <= block_offset) {
        block_size <<= 1;
    }

    size_t index = calculate_log2(block_size);
    if (index >= allocator->num_lists) {
        index = allocator->num_lists - 1;
    }

    block->next = allocator->free_lists[index];
    allocator->free_lists[index] = block;
}