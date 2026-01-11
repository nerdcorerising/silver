// Silver Runtime Library
// Provides core runtime functions for Silver programs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#define SILVER_EXPORT __declspec(dllexport)
#else
#define SILVER_EXPORT __attribute__((visibility("default")))
#endif

// Object header for reference counting
// Placed immediately before the object data in memory
struct SilverObjectHeader {
    int32_t refCount;
};

extern "C" {

// Print a string with newline
SILVER_EXPORT void silver_print_string(const char* s) {
    printf("%s\n", s);
}

// Print an integer with newline
SILVER_EXPORT void silver_print_int(int n) {
    printf("%d\n", n);
}

// Print a float with newline
SILVER_EXPORT void silver_print_float(double f) {
    printf("%f\n", f);
}

// Compare two strings for equality
// Returns 1 if equal, 0 if not
SILVER_EXPORT int silver_strcmp(const char* a, const char* b) {
    return strcmp(a, b) == 0 ? 1 : 0;
}

// Allocate memory with ref count header (initial ref count = 1)
SILVER_EXPORT void* silver_alloc(size_t size) {
    SilverObjectHeader* header = (SilverObjectHeader*)malloc(sizeof(SilverObjectHeader) + size);
    if (!header) return nullptr;
    header->refCount = 1;
    return header + 1;  // Return pointer to data after header
}

// Increment reference count
SILVER_EXPORT void silver_retain(void* ptr) {
    if (!ptr) return;
    SilverObjectHeader* header = ((SilverObjectHeader*)ptr) - 1;
    header->refCount++;
}

// Decrement reference count and free if zero
SILVER_EXPORT void silver_release(void* ptr) {
    if (!ptr) return;
    SilverObjectHeader* header = ((SilverObjectHeader*)ptr) - 1;
    if (--header->refCount == 0) {
        free(header);
    }
}

// Get current reference count (for debugging)
SILVER_EXPORT int silver_refcount(void* ptr) {
    if (!ptr) return 0;
    SilverObjectHeader* header = ((SilverObjectHeader*)ptr) - 1;
    return header->refCount;
}

// Legacy free (for non-ref-counted allocations)
SILVER_EXPORT void silver_free(void* ptr) {
    if (!ptr) return;
    SilverObjectHeader* header = ((SilverObjectHeader*)ptr) - 1;
    free(header);
}

}
