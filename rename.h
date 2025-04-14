#include <stdlib.h>
#include <stdio.h>




//_WIN32 IDK if it works rn 
#if defined(_WIN32)
#include <windows.h>
#include <stdint.h>
#include <malloc.h>
static LARGE_INTEGER frequency;
static CRITICAL_SECTION csMalloc;
static CRITICAL_SECTION csFree;
static CRITICAL_SECTION csRealloc;
__attribute__((constructor)) static void init_frequency() {
    QueryPerformanceFrequency(&frequency);
    InitializeCriticalSection(&csMalloc);
    InitializeCriticalSection(&csFree);
    InitializeCriticalSection(&csRealloc);
}
LARGE_INTEGER get_time_ns() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter;
}
#define OSSIZEOFMALLOC(ptr) (uint64_t) _msize(ptr)
#define LOCKM EnterCriticalSection(&csMalloc)
#define UNLOCKM LeaveCriticalSection(&csMalloc)
#define LOCKF EnterCriticalSection(&csFree)
#define UNLOCKF LeaveCriticalSection(&csFree)
#define LOCKR EnterCriticalSection(&csRealloc)
#define UNLOCKR LeaveCriticalSection(&csRealloc)
#define GETTHREAD (uint64_t)GetCurrentThreadId()
#define TIMETYPE LARGE_INTEGER
#define THREADTYPE DWORD
#define TIME_N get_time_ns()
uint64_t getTimeDif(TIMETYPE t1,TIMETYPE t2){
    uint64_t time1 = (uint64_t)(t1.QuadPart * 1000000000ULL / frequency.QuadPart);
    uint64_t time2 = (uint64_t)(t2.QuadPart * 1000000000ULL / frequency.QuadPart);    
    return time2-time1;
}
#elif defined(__MACH__)
#include <mach/mach_time.h>
#include <unistd.h>
#include <pthread.h>
#include <malloc/malloc.h>
static mach_timebase_info_data_t info = {0};
static pthread_mutex_t csMalloc = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t csFree = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t csRealloc = PTHREAD_MUTEX_INITIALIZER;

#define LOCKM  pthread_mutex_lock(&csMalloc)
#define UNLOCKM pthread_mutex_unlock(&csMalloc)
#define LOCKF pthread_mutex_lock(&csFree)
#define OSSIZEOFMALLOC(ptr) malloc_size(ptr)
#define GETTHREAD ({ \
    uint64_t thread;\
    pthread_threadid_np(NULL, &thread);\
    thread;\
    })
#define UNLOCKF pthread_mutex_unlock(&csFree)
#define LOCKR pthread_mutex_lock(&csRealloc)
#define UNLOCKR pthread_mutex_unlock(&csRealloc)
__attribute__((constructor)) static void init_timebase_info() {
    mach_timebase_info(&info);
}
uint64_t get_time_ns() {
    return mach_absolute_time();
}
#define TIMETYPE uint64_t
#define THREADTYPE uint64_t
#define TIME_N get_time_ns()
uint64_t getTimeDif(TIMETYPE t1, TIMETYPE t2){
    uint64_t time1 =(t1*info.numer)/info.denom;
    uint64_t time2 =(t2*info.numer)/info.denom;
    return time2-time1;
}
#elif defined(__linux__)
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/syscall.h>
static pthread_mutex_t csMalloc = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t csFree = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t csRealloc = PTHREAD_MUTEX_INITIALIZER;
#define OSSIZEOFMALLOC(ptr) malloc_usable_size(ptr)
#define LOCKM  pthread_mutex_lock(&csMalloc)
#define UNLOCKM pthread_mutex_unlock(&csMalloc)
#define LOCKF pthread_mutex_lock(&csFree)
#define UNLOCKF pthread_mutex_unlock(&csFree)
#define LOCKR pthread_mutex_lock(&csRealloc)
#define UNLOCKR pthread_mutex_unlock(&csRealloc)
#define GETTHREAD ((uint64_t)syscall(SYS_gettid))

struct timespec get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}
#define TIMETYPE struct timespec
#define TIME_N get_time_ns()
#define THREADTYPE uint64_t
uint64_t getTimeDif(TIMETYPE t1, TIMETYPE t2){
    uint64_t time1=(uint64_t)t1.tv_sec * 1000000000ULL + (uint64_t)t1.tv_nsec;
    uint64_t time2=(uint64_t)t2.tv_sec * 1000000000ULL + (uint64_t)t2.tv_nsec;
    return time2-time1;
}
#endif


#define CMALLOC(size) malloc(size)
#define CFREE(ptr) free(ptr)
#define CREALLOC(ptr,size) realloc(ptr,size)
#define CCALLOC(info,size) calloc(info,size)
#define CSIZEOFMALLOC(ptr) OSSIZEOFMALLOC(ptr)
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
    double internal_fragmentation = (double)(CSIZEOFMALLOC(ptr) - size)/CSIZEOFMALLOC(ptr) ;\
    LOCKR;\
    FILE *fp = fopen("realloctime.txt", "a");\
    if(fp){\
    fprintf(fp,"TYPEOFREALLOC: %s , TID: %llu  , TIME: %llu ns , Fragmentation: %.2f%% \n",MALLOCLOGGING,GETTHREAD,getTimeDif(ts1,ts2),internal_fragmentation*100);\
    fclose(fp);\
    fp=NULL;\
    UNLOCKR;\
    }\
    ptr;\
 })
 #define DCALLOC(info,size)({\
    TIMETYPE ts1=TIME_N;\
    void *ptr = CCALLOC(info,size);   \
    TIMETYPE ts2=TIME_N\
    double internal_fragmentation = (double)(CSIZEOFMALLOC(ptr) - size)/CSIZEOFMALLOC(ptr) ;\
    LOCKM;\
    FILE *fp = fopen("malloctime.txt", "a");\
    if(fp){\
    fprintf(fp,"TYPEOFMALLOC: %s , TID: %lu  , TIME: %llu ns , Fragmentation: %.2f%% \n",MALLOCLOGGING,GETTHREAD,getTimeDif(ts1,ts2),internal_fragmentation*100);\
    fclose(fp);\
    fp=NULL;\
    UNLOCKM;\
    }\
    ptr;\
 })
#define DMALLOC(size)({\
    TIMETYPE ts1=TIME_N;\
    void *ptr = CMALLOC(size);   \
    TIMETYPE ts2=TIME_N;\
    double internal_fragmentation = (double)(CSIZEOFMALLOC(ptr) - size)/CSIZEOFMALLOC(ptr) ;\
    LOCKM;\
    FILE *fp = fopen("malloctime.txt", "a");\
    if(fp){\
    fprintf(fp,"TYPEOFMALLOC: %s , TID: %llu  , TIME: %llu ns , Fragmentation: %.2f%% \n",MALLOCLOGGING,GETTHREAD,getTimeDif(ts1,ts2),internal_fragmentation*100);\
    fclose(fp);\
    fp=NULL;\
    UNLOCKM;\
    }\
    ptr;\
 })
#define DFREE(ptr)\
do{\
    TIMETYPE ts1=TIME_N;\
free(ptr);   \
TIMETYPE ts2=TIME_N;\
LOCKF;\
FILE *fp = fopen("freetime.txt", "a");\
if(fp){\
fprintf(fp,"TYPEOFMALLOC: %s , TID: %llu , TIME: %llu ns \n",MALLOCLOGGING,GETTHREAD,getTimeDif(ts1,ts2));\
fclose(fp);\
fp=NULL;\
UNLOCKF;\
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
