#include <stddef.h>

typedef struct Allocator {
    void* start;
    size_t total_size;
    void* free_blocks;
} Allocator;

typedef struct FreeBlock {
    size_t block_size;
    struct FreeBlock* next_block;
} FreeBlock;

Allocator* create_allocator(void* memory_pool, size_t pool_size) {
    if (memory_pool == NULL || pool_size < sizeof(FreeBlock)) {
        return NULL;
    }

    Allocator* allocator = (Allocator*)memory_pool;
    allocator->start = (char*)memory_pool + sizeof(Allocator);
    allocator->total_size = pool_size - sizeof(Allocator);
    allocator->free_blocks = allocator->start;

    FreeBlock* first_block = (FreeBlock*)allocator->start;
    first_block->block_size = allocator->total_size;
    first_block->next_block = NULL;

    return allocator;
}

void destroy_allocator(Allocator* allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->start = NULL;
    allocator->total_size = 0;
    allocator->free_blocks = NULL;
}

void* allocate_memory(Allocator* allocator, size_t request_size) {
    if (allocator == NULL || request_size == 0) {
        return NULL;
    }

    FreeBlock* previous = NULL;
    FreeBlock* current = (FreeBlock*)allocator->free_blocks;

    while (current != NULL) {
        if (current->block_size >= request_size + sizeof(FreeBlock)) {
            if (current->block_size > request_size + sizeof(FreeBlock)) {
                FreeBlock* remaining_block = (FreeBlock*)((char*)current + sizeof(FreeBlock) + request_size);
                remaining_block->block_size = current->block_size - request_size - sizeof(FreeBlock);
                remaining_block->next_block = current->next_block;

                current->block_size = request_size;
                current->next_block = remaining_block;
            }

            if (previous != NULL) {
                previous->next_block = current->next_block;
            } else {
                allocator->free_blocks = current->next_block;
            }

            return (char*)current + sizeof(FreeBlock);
        }

        previous = current;
        current = current->next_block;
    }

    return NULL;
}

void free_memory(Allocator* allocator, void* memory) {
    if (allocator == NULL || memory == NULL) {
        return;
    }

    FreeBlock* block_to_free = (FreeBlock*)((char*)memory - sizeof(FreeBlock));
    block_to_free->next_block = (FreeBlock*)allocator->free_blocks;
    allocator->free_blocks = block_to_free;
}