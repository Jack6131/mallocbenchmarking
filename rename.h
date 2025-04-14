#include <stdlib.h>
#include <stdio.h>

#include <malloc/malloc.h>
#include <pthread.h>


#if defined(_WIN32)
#include <windows.h>
#include <stdint.h>
static LARGE_INTEGER frequency;

__attribute__((constructor)) static void init_frequency() {
    QueryPerformanceFrequency(&frequency);
}
LARGE_INTEGER get_time_ns() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter;
}
#define TIMETYPE LARGE_INTEGER
#define TIME_N get_time_ns()
uint64_t getTimeDif(TIMETYPE t1,TIMETYPE t2){
    uint64_t time1 = (uint64_t)(t1.QuadPart * 1000000000ULL / frequency.QuadPart);
    uint64_t time2 = (uint64_t)(t2.QuadPart * 1000000000ULL / frequency.QuadPart);    
    return time2-time1;
}
#elif defined(__MACH__)
#include <mach/mach_time.h>
#include <unistd.h>
static mach_timebase_info_data_t info = {0};

__attribute__((constructor)) static void init_timebase_info() {
    mach_timebase_info(&info);
}
uint64_t get_time_ns() {
    return mach_absolute_time();
}
#define TIMETYPE uint64_t
#define TIME_N get_time_ns()
uint64_t getTimeDif(TIMETYPE t1, TIMETYPE t2){
    printf("numer:%u  denom:%u\n",info.numer,info.denom);
    uint64_t time1 =(t1*info.numer)/info.denom;
    uint64_t time2 =(t2*info.numer)/info.denom;
    return time2-time1;
}
#elif defined(__linux__)
#include <time.h>
#include <unistd.h>
#include <stdint.h>
struct timespec get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}
#define TIMETYPE struct timespec
#define TIME_N get_time_ns()
uint64_t getTimeDif(TIMETYPE t1, TIMETYPE t2){
    uint64_t time1=(uint64_t)t1.tv_sec * 1000000000ULL + (uint64_t)t1.tv_nsec;
    uint64_t time2=(uint64_t)t2.tv_sec * 1000000000ULL + (uint64_t)t2.tv_nsec;
    return time2-time1;
}
#endif
static pthread_mutex_t dmalloc_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dmfree_mutex = PTHREAD_MUTEX_INITIALIZER;

#define CMALLOC(size) malloc(size)
#define CFREE(ptr) free(ptr)
#define CREALLOC(ptr,size) realloc(ptr,size)
#define CCALLOC(info,size) calloc(info,size)
#define CSIZEOFMALLOC(ptr) malloc_size(ptr)
#define MALLOCLOGGING "Default Malloc"
#ifdef USE_JEMALLOC
    #include <jemalloc/jemalloc.h>
    #undef CMALLOC
    #undef CFREE
    #undef CCALLOC
    #undef CREALLOC
    #undef MALLOCLOGGING
    #undef CSIZEOFMALLOC
    #define MALLOCLOGGING "JE Malloc"
    #define CMALLOC(size) je_malloc(size)
    #define CFREE(ptr) je_free(ptr)
    #define CSIZEOFMALLOC(ptr) je_malloc_usable_size(ptr)
    #define CREALLOC(ptr,size) je_realloc(ptr,size)
    #define CMALLOCX(size,flag) je_mallocx(size,flag)
    #define CCALLOC(info,size) je_calloc(info,size)
#endif
#define DREALLOC(ptr,size)({\
    TIMETYPE ts1=TIME_N;\
    void *ptr = CREALLOC(ptr,size);   \
    TIMETYPE ts2=TIME_N;\
    uint64_t tid;\
    pthread_threadid_np(NULL, &tid);\
    double internal_fragmentation = (double)(CSIZEOFMALLOC(ptr) - size)/CSIZEOFMALLOC(ptr) ;\
    pthread_mutex_lock(&dmalloc_mutex);\
    FILE *fp = fopen("realloctime.txt", "a");\
    if(fp){\
    fprintf(fp,"TYPEOFREALLOC: %s , TID: %llu  , TIME: %llu ns , Fragmentation: %.2f%% \n",MALLOCLOGGING,tid,getTimeDif(ts1,ts2),internal_fragmentation*100);\
    fclose(fp);\
    fp=NULL;\
    pthread_mutex_unlock(&dmalloc_mutex);\
    }\
    ptr;\
 })
 #define DCALLOC(info,size)({\
    TIMETYPE ts1=TIME_N;\
    void *ptr = CCALLOC(info,size);   \
    TIMETYPE ts2=TIME_N\
    uint64_t tid;\
    pthread_threadid_np(NULL, &tid);\
    double internal_fragmentation = (double)(CSIZEOFMALLOC(ptr) - size)/CSIZEOFMALLOC(ptr) ;\
    pthread_mutex_lock(&dmalloc_mutex);\
    FILE *fp = fopen("malloctime.txt", "a");\
    if(fp){\
    fprintf(fp,"TYPEOFMALLOC: %s , TID: %llu  , TIME: %llu ns , Fragmentation: %.2f%% \n",MALLOCLOGGING,tid,getTimeDif(ts1,ts2),internal_fragmentation*100);\
    fclose(fp);\
    fp=NULL;\
    pthread_mutex_unlock(&dmalloc_mutex);\
    }\
    ptr;\
 })
#define DMALLOC(size)({\
    TIMETYPE ts1=TIME_N;\
    void *ptr = CMALLOC(size);   \
    TIMETYPE ts2=TIME_N;\
    uint64_t tid;\
    pthread_threadid_np(NULL, &tid);\
    double internal_fragmentation = (double)(CSIZEOFMALLOC(ptr) - size)/CSIZEOFMALLOC(ptr) ;\
    pthread_mutex_lock(&dmalloc_mutex);\
    FILE *fp = fopen("malloctime.txt", "a");\
    if(fp){\
    fprintf(fp,"TYPEOFMALLOC: %s , TID: %llu  , TIME: %llu ns , Fragmentation: %.2f%% \n",MALLOCLOGGING,tid,getTimeDif(ts1,ts2),internal_fragmentation*100);\
    fclose(fp);\
    fp=NULL;\
    pthread_mutex_unlock(&dmalloc_mutex);\
    }\
    ptr;\
 })
#define DFREE(ptr)\
do{\
    TIMETYPE ts1=TIME_N;\
free(ptr);   \
TIMETYPE ts2=TIME_N;\
uint64_t tid;\
pthread_threadid_np(NULL, &tid);\
pthread_mutex_lock(&dmfree_mutex);\
FILE *fp = fopen("freetime.txt", "a");\
if(fp){\
fprintf(fp,"TYPEOFMALLOC: %s , TID: %llu , TIME: %llu ns \n",MALLOCLOGGING,tid,getTimeDif(ts1,ts2));\
fclose(fp);\
fp=NULL;\
pthread_mutex_unlock(&dmfree_mutex);\
} \
}while(0)

#ifdef DEBUG
#define MALLOC(size) DMALLOC(size)
#define FREE(ptr) DFREE(ptr)
#define REALLOC(ptr,size) DREALLOC(ptr,size)
#define CALLOC(ptr,size) DCALLOC(ptr,size)
#else
#define MALLOC(size) CMALLOC(size)
#define FREE(ptr) CFREE(ptr)
#define REALLOC(ptr,size) CREALLOC(ptr,size)
#define CALLOC(ptr,size) CCALLOC(ptr,size)
#endif
#define SIZEOFMALLOC(ptr) CSIZEOFMALLOC(ptr)
