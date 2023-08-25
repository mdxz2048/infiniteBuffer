/**
 * @file types.h
 * @author lzp (lvzhipeng@mdxz.com.cn)
 * @brief 常用数据类型、返回值、宏工具
 * @version 0.1
 * @date 2022-03-25
 * 
 * @copyright Copyright (c) MDXZ 2022
 * 
 */
#ifndef __TYPES_H_
#define __TYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************类型定义*****************************/
/* Platform int length */
#define PLATFORM_INT_LEN    32

#ifndef VOID
#define VOID                void
#endif

/* Common data types */
#ifndef BYTE
#define BYTE                unsigned char
#endif

#ifndef BOOL
#define BOOL                unsigned char
#endif

#ifndef CHAR
#define CHAR                char
#endif

#ifndef SCHAR
#define SCHAR               signed char
#endif

#ifndef UCHAR
#define UCHAR               unsigned char
#endif

#ifndef INT8
#define INT8                signed char
#endif

#ifndef UINT8
#define UINT8               unsigned char
#endif

#ifndef INT16
#define INT16               short
#endif

#ifndef UINT16
#define UINT16              unsigned short
#endif

#ifndef DWORD
#define DWORD                unsigned int
#endif

#if (PLATFORM_INT_LEN == 16)

#ifndef INT32
#define INT32               long
#endif

#ifndef UINT32
#define UINT32              unsigned long
#endif

#elif (PLATFORM_INT_LEN == 32)

#ifndef INT32
#define INT32               int
#endif

#ifndef UINT32
#define UINT32              unsigned int
#endif

#endif /* PLATFORM_INT_LEN */

#ifndef UINT64
#define UINT64              unsigned long long
#endif

#ifndef DOUBLE
#define DOUBLE              double
#endif

#ifndef FLOAT
#define FLOAT               float
#endif

#ifdef __cplusplus  // close out "C" linkage in case of c++ compiling
}
#endif
/*****************************通用宏定义*****************************/

#ifndef NULL
#define NULL ((void*) 0)
#endif

/**
 * @brief 函数返回值
 * 
 */
typedef enum 
{
 SUCCESS = 0,
 FAILURE = -1,
}ERR_CODE_e;

/**
 * @brief MDXZ设备(应用)状态
 */
typedef enum 
{
    DEV_TRUE  = 0,
    DEV_FALSE = -1,
}DEV_STATUS_e;

typedef struct
{
    char *name;
    char *version;
    char *platform;
    char *author;
    char *description;
} LIB_INFO_t;

/*****************************常用工具宏定义*****************************/

//位操作
#define GETBIT(A, B) ((A >> B) & 1)//获取A的第B位的值;
#define SETBIT(T, B, V) (T = V ? T | (1 << B) : T & ~(1 << B))//设置T的第B位为V(0/1)
//MAX&MIN
#define MAX(x, y) ({\
        typeof(x) _x = (x); \
        typeof(y) _y = (y); \
        (void)(&_x == &_y); \
        _x > _y ? _x : _y; \
})

#define MIN(X,Y) ({\
	typeof (X) x_ = (X);\
	typeof (Y) y_ = (Y);\
	(x_ < y_) ? x_ : y_; })

//数组元素个数
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))

//printf打印
#define DEBUG_PRINTF
#ifdef DEBUG_PRINTF
#define debug_printf(format, ...) printf("\033[1;32mFile: "__FILE__",Line: %05d: FUNC: %s\t\033[0m" format "\n", __LINE__,__FUNCTION__ ,##__VA_ARGS__)
#else
#define debug_printf
#endif


#ifdef __cplusplus
}
#endif

#endif