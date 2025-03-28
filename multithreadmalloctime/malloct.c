#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach_time.h>
#include <time.h>

#define ITERATION 1000000
#define MAX_SIZE 1024

// Thread function to measure malloc/free time
void* threadfunction(void* arg) {
    int (*matrix)[2] = (int (*)[2])arg; // Cast the argument to a 2D array
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    for (int i = 0; i < ITERATION; i++) {
        size_t size = rand() % MAX_SIZE + 1;

        uint64_t ts1 = mach_absolute_time();
        int* ptr = malloc(size);
        uint64_t ts2 = mach_absolute_time();
        free(ptr);
        uint64_t ts3 = mach_absolute_time();

        uint64_t malloc_time = (ts2 - ts1) * info.numer / info.denom;
        uint64_t free_time = (ts3 - ts2) * info.numer / info.denom;

        matrix[i][0] = (int)malloc_time; // Store malloc time
        matrix[i][1] = (int)free_time;  // Store free time
    }

    return NULL; // Return NULL to match the expected signature
}

int main() {
    FILE *fptr, *fptr2;
    fptr2 = fopen("timeofmalloc.txt", "w");
    fptr = fopen("timeofFree.txt", "w");

    if (fptr == NULL || fptr2 == NULL) {
        printf("Error opening file!\n");
        return 1;
    }

    srand(time(NULL)); // Seed the random number generator

    // Allocate memory for matrices
    int (*matrix1)[2] = malloc(ITERATION * 2 * sizeof(int));
    int (*matrix2)[2] = malloc(ITERATION * 2 * sizeof(int));
    int (*matrix3)[2] = malloc(ITERATION * 2 * sizeof(int));
    int (*matrix4)[2] = malloc(ITERATION * 2 * sizeof(int));
    int (*matrix5)[2] = malloc(ITERATION * 2 * sizeof(int));

    if (matrix1 == NULL || matrix2 == NULL || matrix3 == NULL || matrix4 == NULL || matrix5 == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

  
    pthread_t tid1, tid2, tid3, tid4, tid5;
    pthread_create(&tid1, NULL, threadfunction, matrix1);
    pthread_create(&tid2, NULL, threadfunction, matrix2);
    pthread_create(&tid3, NULL, threadfunction, matrix3);
    pthread_create(&tid4, NULL, threadfunction, matrix4);
    pthread_create(&tid5, NULL, threadfunction, matrix5);


    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
    pthread_join(tid5, NULL);

    
    fprintf(fptr, "THREAD 1\n");
    fprintf(fptr2, "THREAD 1\n");
    for (int p = 0; p < ITERATION; p++) {
        fprintf(fptr2, "%d\n", matrix1[p][0]);
        fprintf(fptr, "%d\n", matrix1[p][1]);
    }

    fprintf(fptr, "THREAD 2\n");
    fprintf(fptr2, "THREAD 2\n");
    for (int p = 0; p < ITERATION; p++) {
        fprintf(fptr2, "%d\n", matrix2[p][0]);
        fprintf(fptr, "%d\n", matrix2[p][1]);
    }

    fprintf(fptr, "THREAD 3\n");
    fprintf(fptr2, "THREAD 3\n");
    for (int p = 0; p < ITERATION; p++) {
        fprintf(fptr2, "%d\n", matrix3[p][0]);
        fprintf(fptr, "%d\n", matrix3[p][1]);
    }

    fprintf(fptr, "THREAD 4\n");
    fprintf(fptr2, "THREAD 4\n");
    for (int p = 0; p < ITERATION; p++) {
        fprintf(fptr2, "%d\n", matrix4[p][0]);
        fprintf(fptr, "%d\n", matrix4[p][1]);
    }

    fprintf(fptr, "THREAD 5\n");
    fprintf(fptr2, "THREAD 5\n");
    for (int p = 0; p < ITERATION; p++) {
        fprintf(fptr2, "%d\n", matrix5[p][0]);
        fprintf(fptr, "%d\n", matrix5[p][1]);
    }

    // Free allocated memory
    free(matrix1);
    free(matrix2);
    free(matrix3);
    free(matrix4);
    free(matrix5);

    // Close files
    fclose(fptr);
    fclose(fptr2);

    printf("All threads have finished.\n");

    return 0;
}