// Silver Runtime Library
// Provides core runtime functions for Silver programs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define SILVER_EXPORT __declspec(dllexport)
#else
#define SILVER_EXPORT __attribute__((visibility("default")))
#endif

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

// Allocate memory
SILVER_EXPORT void* silver_alloc(size_t size) {
    return malloc(size);
}

// Free memory
SILVER_EXPORT void silver_free(void* ptr) {
    free(ptr);
}

}
