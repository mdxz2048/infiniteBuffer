#ifndef _LIB_INFINITE_BUFFER_H_
#define _LIB_INFINITE_BUFFER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include "types.h"

#define CACHE_LINE_SIZE 64

typedef struct
{
    u_int32_t bufLen;               //buffer length
    char *bufStart;                 //buffer start address
    char *bufEnd;                   //buffer end address

    bool isFull;             //is full, update by write thread
    char *writePtr;      //write pointer, 
    char *waterMark;     //water mark
    char _pad1[CACHE_LINE_SIZE - sizeof(u_int32_t) - 5 * sizeof(char*) - sizeof(bool)]; // padding to avoid false sharing

    atomic_bool isEmpty;            //is empty, update by read thread
    atomic_bool isReading;          //is reading, 
    atomic_uintptr_t readPtr;       //read pointer
    atomic_bool isExclusive;        //is exclusive, update by read thread
    pthread_t   exclusiveOwner;    //exclusive owner, update by read thread
    char _pad2[CACHE_LINE_SIZE - 4 * sizeof(atomic_bool) - sizeof(atomic_uintptr_t)]; // padding to avoid false sharing

} infiniteBuffer_t;

ERR_CODE_e lib_infinite_buffer_create(infiniteBuffer_t *buffer, size_t bufLen);
ERR_CODE_e lib_infinite_buffer_destroy(infiniteBuffer_t *buffer);
ERR_CODE_e lib_infinite_buffer_write(infiniteBuffer_t *buffer, const char *data, size_t len);
ERR_CODE_e lib_infinite_buffer_write_wait(infiniteBuffer_t *buffer, const char *data, size_t len);
int32_t lib_infinite_buffer_read(infiniteBuffer_t *buffer, char *data, size_t len);
int32_t lib_infinite_buffer_advance_readptr(infiniteBuffer_t *buffer, size_t len);
bool lib_infinite_buffer_isEmpty(infiniteBuffer_t *buffer);
bool lib_infinite_buffer_isFull(infiniteBuffer_t *buffer);
ERR_CODE_e lib_infinite_buffer_reads_set_exclusive(infiniteBuffer_t *buffer);
ERR_CODE_e lib_infinite_buffer_reads_unset_exclusive(infiniteBuffer_t *buffer);


#endif