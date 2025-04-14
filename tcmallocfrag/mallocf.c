#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <malloc/malloc.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#define NUM_ALLOCS 1000
#define MAX_SIZE 1024
int flag =0;



void report_memory(void) {
  struct task_basic_info info;
  mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
  kern_return_t kerr = task_info(mach_task_self(),
                                 TASK_BASIC_INFO,
                                 (task_info_t)&info,
                                 &size);
  if( kerr == KERN_SUCCESS ) {
    if(info.resident_size / 1048576 >5){
        flag=1;
    }
  } else {
    printf("Error with task_info(): %s", mach_error_string(kerr));
  }
}

int main() {
    srand(time(NULL));
    void* pointers[NUM_ALLOCS];
    struct rlimit limit;
    // Open a file to log successful allocations
    FILE *fptr = fopen("succesfulruns.txt", "w");
    if (fptr == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    FILE *fptr2 = fopen("memorycheck.txt", "w");
    if (fptr == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    // Stress test malloc until crash
    size_t allocation_count = 0;
    
    /*  */
    size_t total_allocated=0;
    size_t actual_used =0;

        for (int i = 0; i < NUM_ALLOCS; i++) {
            size_t size = rand() % MAX_SIZE +1;
            pointers[i] = malloc(size);
            total_allocated += size;
            actual_used += malloc_size(pointers[i]);
        }
       
        printf("ACTUALLY USED:%zu\n",actual_used);
        printf("DESIRED ALLOCATION:%zu\n" ,total_allocated);
        report_memory();
        // Log successful allocation
        fprintf(fptr, "%zu\n", allocation_count);
        allocation_count++;
        double internal_fragmentation = (double)(actual_used - total_allocated) / actual_used;
        printf("Internal Fragmentation: %.2f%%\n", internal_fragmentation * 100);
    

    // Close the file
    fclose(fptr);
   

    return 0;
}