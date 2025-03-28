#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <jemalloc/jemalloc.h>

#define NUM_ALLOCS 1000
#define MAX_SIZE 1024

void measure_internal_fragmentation() {
    srand(time(NULL));
    void* pointers[NUM_ALLOCS];
    size_t total_allocated = 0;  // Total memory requested
    size_t total_used = 0;       // Total memory actually allocated

    // Allocate memory
    for (int i = 0; i < NUM_ALLOCS; i++) {
        size_t size = rand() % MAX_SIZE + 1;  // Random size between 1 and MAX_SIZE
        pointers[i] = je_malloc(size);        // Use jemalloc's malloc
        total_allocated += size;
        total_used += je_malloc_usable_size(pointers[i]);  // Get actual allocated size
    }


    // Calculate internal fragmentation
    double internal_fragmentation = (double)(total_used - total_allocated) / total_used;
    printf("Total Requested Memory: %zu bytes\n", total_allocated);
    printf("Total Actual Memory Used: %zu bytes\n", total_used);
    printf("Internal Fragmentation: %.2f%%\n", internal_fragmentation * 100);
}

int main() {

    measure_internal_fragmentation();
    return 0;
}