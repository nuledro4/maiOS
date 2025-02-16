#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct AllocatorAPI {
    void* (*create)(void* addr, size_t size);
    void* (*alloc)(void* allocator, size_t size);
    void (*free)(void* allocator, void* ptr);
    void (*destroy)(void* allocator);
} AllocatorAPI;

void* default_create(void* memory, size_t size) {
    (void)size;
    return memory;
}

void* default_alloc(void* allocator, size_t size) {
    (void)allocator;
    uint32_t* block = mmap(NULL, size + sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (block == MAP_FAILED) {
        return NULL;
    }
    *block = (uint32_t)(size + sizeof(uint32_t));
    return block + 1;
}

void default_free(void* allocator, void* memory) {
    (void)allocator;
    if (!memory) return;
    uint32_t* block = (uint32_t*)memory - 1;
    munmap(block, *block);
}

void default_destroy(void* allocator) {
    (void)allocator;
}

void load_allocator(const char* lib_path, AllocatorAPI* api) {
    void* lib_handle = dlopen(lib_path, RTLD_LOCAL | RTLD_NOW);
    if (!lib_path || !lib_path[0] || !lib_handle) {
        write(STDERR_FILENO, "WARNING: Using default allocator\n", 34);
        api->create = default_create;
        api->alloc = default_alloc;
        api->free = default_free;
        api->destroy = default_destroy;
        return;
    }

    api->create = dlsym(lib_handle, "create_allocator");
    api->alloc = dlsym(lib_handle, "allocate_memory");
    api->free = dlsym(lib_handle, "free_memory");
    api->destroy = dlsym(lib_handle, "destroy_allocator");

    if (!api->create || !api->alloc || !api->free || !api->destroy) {
        write(STDERR_FILENO, "ERROR: Failed loading allocator functions\n", 43);
        dlclose(lib_handle);
        api->create = default_create;
        api->alloc = default_alloc;
        api->free = default_free;
        api->destroy = default_destroy;
    }
}

void print_message(const char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

void print_address(const char* label, int index, void* address) {
    char buffer[64];
    int len = 0;

    while (*label) {
        buffer[len++] = *label++;
    }

    if (index < 10) {
        buffer[len++] = '0' + index;
    } else {
        buffer[len++] = '0' + (index / 10);
        buffer[len++] = '0' + (index % 10);
    }

    char* ad = " address: ";
    while (*ad) {
        buffer[len++] = *ad++;
    }

    uintptr_t addr = (uintptr_t)address;
    for (int i = (sizeof(uintptr_t) * 2) - 1; i >= 0; --i) {
        int nibble = (addr >> (i * 4)) & 0xF;
        buffer[len++] = (nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10));
    }

    buffer[len++] = '\n';
    write(STDOUT_FILENO, buffer, len);
}

int main(int argc, char** argv) {
    const char* lib_path = (argc > 1) ? argv[1] : NULL;
    AllocatorAPI allocator_api;
    load_allocator(lib_path, &allocator_api);

    size_t pool_size = 4096;
    void* pool = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pool == MAP_FAILED) {
        print_message("ERROR: Memory pool allocation failed\n");
        return EXIT_FAILURE;
    }

    void* allocator = allocator_api.create(pool, pool_size);
    if (!allocator) {
        print_message("ERROR: Allocator initialization failed\n");
        munmap(pool, pool_size);
        return EXIT_FAILURE;
    }

    size_t block_sizes[] = {12, 13, 24, 40, 56, 100, 120, 400};
    void* blocks[8];

    for (int i = 0; i < 8; ++i) {
        blocks[i] = allocator_api.alloc(allocator, block_sizes[i]);
        if (!blocks[i]) {
            print_message("ERROR: Memory allocation failed\n");
            break;
        }
        print_address("block №", i + 1, blocks[i]);
    }
    print_message("INFO: Memory allocation - SUCCESS\n");

    for (int i = 0; i < 8; ++i) {
        if (blocks[i]) {
            allocator_api.free(allocator, blocks[i]);
        }
    }
    print_message("INFO: Memory freed\n");

    allocator_api.destroy(allocator);
    print_message("INFO: Allocator destroyed\n");

    return EXIT_SUCCESS;
}