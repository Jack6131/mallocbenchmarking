#include <stdio.h>
#include<stdlib.h>
#include <sys/time.h>
#include <mach/mach_time.h>



int main(){
FILE *fptr,*fptr2;
fptr2 = fopen("timeofmalloc.txt", "w");
fptr = fopen("timeofFree.txt", "w");
if (fptr == NULL) {
    printf("Error opening file!\n");
    return 1; 
}
if (fptr2 == NULL) {
    printf("Error opening file!\n");
    return 1; 
}
struct timeval tvs,tve;
mach_timebase_info_data_t info;
mach_timebase_info(&info);

for(int i=0;i<100000000;i++){

    uint64_t ts1 = mach_absolute_time();

    
int *ptr = (int *)malloc(sizeof(int) * 1000000);



//fprintf(fptr2,"Memory address1: %p\n", ptr); 

//fprintf(fptr2,"Memory address2: %p\n", ptr2); 
//fprintf(fptr2,"Memory address 1 after realloc: %p\n", ptr); 
//fprintf(fptr2,"Memory address 2 after realloc: %p\n", ptr2); 
uint64_t ts2 = mach_absolute_time();
free(ptr);
uint64_t ts3 = mach_absolute_time();

//printf("START:%ld MID1:%ld MID2:%ld END%ld\n",tv1.tv_usec,tv2.tv_usec,tv3.tv_usec,tv4.tv_usec);

uint64_t malloc_time = (ts2 - ts1) * info.numer / info.denom;
uint64_t free_time = (ts3 - ts2) * info.numer / info.denom;
fprintf(fptr2,"%llu\n",malloc_time); 
fprintf(fptr,"%llu\n",free_time); 
}
printf("DONE");

}

