/*
 * @Author       : lvzhipeng
 * @Date         : 2023-08-25 10:20:17
 * @LastEditors  : lvzhipeng
 * @LastEditTime : 2023-08-28 11:22:27
 * @FilePath     : /lib_infiniteBuffer/src/lib_infinite_buffer.c
 * @Description  :
 *
 */
#include "lib_infinite_buffer.h"
#include "types.h"

ERR_CODE_e lib_infinite_buffer_create(infiniteBuffer_t *buffer, u_int32_t bufLen)
{
    if (buffer == NULL)
    {
        debug_printf("buffer is NULL\n");
        return FAILURE;
    }
    if (bufLen == 0)
    {
        debug_printf("bufLen is 0\n");
        return FAILURE;
    }

    buffer->bufLen = bufLen;
    buffer->bufStart = (char *)malloc(bufLen);
    if (buffer->bufStart == NULL)
    {
        debug_printf("malloc failed\n");
        return FAILURE;
    }
    memset(buffer->bufStart, 0, bufLen);
    // set buffer end address, bufEnd = bufStart + bufLen
    buffer->bufEnd = buffer->bufStart + bufLen;

    // FOR WRITE
    atomic_store(&buffer->isFull, false);
    atomic_store(&buffer->isWriting, false);
    atomic_store(&buffer->writePtr, (atomic_uintptr_t)buffer->bufStart);
    atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
    // FOR READ
    atomic_store(&buffer->isEmpty, true);
    atomic_store(&buffer->isReading, false);
    atomic_store(&buffer->readPtr, (atomic_uintptr_t)buffer->bufStart);
    debug_printf("[CREATE]  buffer=[%p -> %p], length=[%d]\n", buffer->bufStart, buffer->bufEnd, buffer->bufLen);

    return SUCCESS;
}

ERR_CODE_e lib_infinite_buffer_destroy(infiniteBuffer_t *buffer)
{
    if (buffer == NULL)
    {
        debug_printf("buffer is NULL\n");
        return FAILURE;
    }
    if (buffer->bufStart == NULL)
    {
        debug_printf("buffer->bufStart is NULL\n");
        return FAILURE;
    }
    free(buffer->bufStart);
    buffer->bufStart = NULL;
    buffer->bufEnd = NULL;
    buffer->bufLen = 0;

    return SUCCESS;
}

ERR_CODE_e lib_infinite_buffer_write(infiniteBuffer_t *buffer, const char *data, u_int32_t len)
{
    if (!buffer || !data || len == 0)
    {
        debug_printf("[WRITE] Invalid parameters\n");
        return FAILURE;
    }

    char *writePtr = (char *)atomic_load(&buffer->writePtr);
    char *readPtr = (char *)atomic_load(&buffer->readPtr);
    if (writePtr == readPtr)
    {
        if (atomic_load(&buffer->isFull))
        {
            debug_printf("[WRITE] writePtr == readPtr(%p, %p), buffer is full", writePtr, readPtr);
            return FAILURE;
        }
        else
        {
            memcpy(writePtr, data, len);
            writePtr += len;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, false);
            atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
            debug_printf("[WRITE] newDatalength=[%d], writePtr=[%p]", len, writePtr);
            return SUCCESS;
        }
    }
    else if (writePtr > readPtr)
    {
        int32_t freeLength = buffer->bufEnd - writePtr;
        if (freeLength >= len)
        {
            memcpy(writePtr, data, len);
            writePtr += len;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, false);
            atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
            debug_printf("[WRITE] newDatalength=[%d], writePtr=[%p]", len, writePtr);
            return SUCCESS;
        }
        else
        {
            atomic_store(&buffer->waterMark, (atomic_uintptr_t)writePtr);
            int32_t freeLengthNext = readPtr - buffer->bufStart;
            if (freeLengthNext > len)
            {
                memcpy(buffer->bufStart, data, len);
                atomic_store(&buffer->isFull, false);
                atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
                writePtr = buffer->bufStart + len;
                atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);

                debug_printf("[WRITE] newDatalength=[%d], writePtr=[%p]", len, writePtr);
                return SUCCESS;
            }
            else if (freeLengthNext == len)
            {
                memcpy(buffer->bufStart, data, len);
                writePtr = buffer->bufStart;
                atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
                atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
                atomic_store(&buffer->isFull, true);
                debug_printf("[WRITE] newDatalength=[%d], writePtr=[%p]", len, writePtr);
                return SUCCESS;
            }
            else
            {
                debug_printf("[WRITE] Free is not enough space.\n");
                debug_printf("[WRITE] writePtr > readPtr(%p,%p), freeLength < len(%d, %d)", writePtr, readPtr, freeLength, len);
                return FAILURE;
            }
        }
    }
    else if (writePtr < readPtr)
    {
        int32_t freeLength = readPtr - writePtr;
        if (freeLength > len)
        {
            memcpy(writePtr, data, len);
            writePtr += len;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, false);
            debug_printf("[WRITE] writePtr < readPtr(%p, %p), freeLength > len(%d, %d)\n", writePtr, readPtr, freeLength, len);
            return SUCCESS;
        }
        else if (freeLength == len)
        {
            printf("test");
            memcpy(writePtr, data, len);
            writePtr = buffer->bufStart;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, true);
            debug_printf("[WRITE] writePtr < readPtr(%p, %p), freeLength == len(%d, %d)\n", writePtr, readPtr, freeLength, len);
            return SUCCESS;
        }
        else
        {
            debug_printf("[WRITE] Free is not enough space.\n");
            debug_printf("[WRITE] writePtr < readPtr(%p, %p), freeLength < len(%d, %d)\n", writePtr, readPtr, freeLength, len);
            return FAILURE;
        }
    }
}

int32_t lib_infinite_buffer_read(infiniteBuffer_t *buffer, char *data, u_int32_t len)
{
    if (!buffer || !data || len == 0)
    {
        debug_printf("Invalid parameters\n");
        return -1;
    }

    while (1)
    {
        if (atomic_load(&buffer->isReading))
        {
            continue;
        }
        else
        {
            atomic_store(&buffer->isReading, true);
            break;
        }
    }

    char *readPtr = (char *)atomic_load(&buffer->readPtr);
    char *writePtr = (char *)atomic_load(&buffer->writePtr);
    char *waterMark = (char *)atomic_load(&buffer->waterMark);
    // if (writePtr == readPtr)
    while (writePtr == readPtr)
    {
        if (atomic_load(&buffer->isFull))
        {
            int32_t readableLength = waterMark - readPtr;
            if (readableLength >= len)
            {
                memcpy(data, readPtr, len);
                readPtr += len;
                atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
                atomic_store(&buffer->isEmpty, false);
                debug_printf("[READ] readDataLength=[%d], readPtr=[%p]\n", len, readPtr);
                atomic_store(&buffer->isReading, false);

                return len;
            }
            else
            {
                debug_printf("[READ] writePtr == readPtr(%p, %p), readableLength < len(%d, %d)\n", writePtr, readPtr, readableLength, len);
                atomic_store(&buffer->isReading, false);

                return -1;
            }
        }
        else
        {
            atomic_store(&buffer->isEmpty, true);
            debug_printf("[READ] writePtr == readPtr(%p, %p), set isEmpty = true\n", writePtr, readPtr);
            // debug_printf("[READ] buffer is empty\n");
            atomic_store(&buffer->isReading, false);

            break;
            // return -1;
        }
    }

    if (writePtr > readPtr)
    {
        int32_t readableLength = writePtr - readPtr;
        if (readableLength >= len)
        {
            memcpy(data, readPtr, len);
            readPtr += len;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, false);
            debug_printf("[READ] readDataLength=[%d], readPtr=[%p]\n", len, readPtr);
            atomic_store(&buffer->isReading, false);

            return len;
        }
        else
        {
            debug_printf("[READ] writePtr > readPtr(%p, %p), readableLength < len(%d, %d)\n", writePtr, readPtr, readableLength, len);
            atomic_store(&buffer->isReading, false);

            return -1;
        }
    }
    else if (writePtr < readPtr)
    {
        int32_t readableLength = waterMark - readPtr;
        if (readableLength >= len)
        {
            memcpy(data, readPtr, len);
            readPtr += len;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, false);
            debug_printf("[READ] readDataLength=[%d], readPtr=[%p]\n", len, readPtr);
            atomic_store(&buffer->isReading, false);

            return len;
        }
        else
        {
            debug_printf("[READ] writePtr < readPtr(%p, %p), readableLength < len(%d, %d)\n", writePtr, readPtr, readableLength, len);
            atomic_store(&buffer->isReading, false);

            return -1;
        }
    }

    return SUCCESS;
}

ERR_CODE_e lib_infinite_buffer_peek(infiniteBuffer_t *buffer, char *data, u_int32_t len)
{
    return SUCCESS;
}
