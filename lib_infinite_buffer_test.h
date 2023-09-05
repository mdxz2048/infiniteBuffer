#ifndef __LIB_INFINITE_BUFFER_TEST__H_
#define __LIB_INFINITE_BUFFER_TEST__H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"

#define DOMAIN_SOCK_PATH "/tmp/domain_sock_path"
#define SYNC_VALUE 0x55AA
#define MAX_DATA_SIZE 20480

typedef struct {
    u_int32_t sync; // 0x55AA
    u_int64_t msgCnt;
    u_int32_t len;  
} testDataHeader_t;

#ifdef __cplusplus
}

#endif

#endif