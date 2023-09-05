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


/**
 * @brief RTK平台注册信息
 */
typedef struct
{
    u_int32_t bufLen;               //buffer length
    char *bufStart;                 //buffer start address
    char *bufEnd;                   //buffer end address

    atomic_bool isFull;             //is full, update by write thread
    atomic_bool isWriting;          //is writing, update by write thread
    atomic_uintptr_t writePtr;      //write pointer, 
    atomic_uintptr_t waterMark;     //water mark

    atomic_bool isEmpty;            //is empty, update by read thread
    atomic_bool isReading;          //is reading, 
    atomic_uintptr_t readPtr;       //read pointer


} infiniteBuffer_t;

ERR_CODE_e lib_infinite_buffer_create(infiniteBuffer_t *buffer, size_t bufLen);
ERR_CODE_e lib_infinite_buffer_destroy(infiniteBuffer_t *buffer);
ERR_CODE_e lib_infinite_buffer_write(infiniteBuffer_t *buffer, const char *data, size_t len);
ERR_CODE_e lib_infinite_buffer_write_wait(infiniteBuffer_t *buffer, const char *data, size_t len);
int32_t lib_infinite_buffer_read(infiniteBuffer_t *buffer, char *data, size_t len);
int32_t lib_infinite_buffer_read_wait(infiniteBuffer_t *buffer, char *data, size_t len);

bool lib_infinite_buffer_isEmpty(infiniteBuffer_t *buffer);



#endif