/*
 * @Author       : lvzhipeng
 * @Date         : 2023-08-25 10:20:17
 * @LastEditors  : lvzhipeng
 * @LastEditTime : 2023-09-22 16:12:31
 * @FilePath     : /infiniteBuffer/src/lib_infinite_buffer.c
 * @Description  :
 *
 */
#include "lib_infinite_buffer.h"
#include <stddef.h>
#include "types.h"

#define PRINTF_READ_INFO 0
#define PRINTF_WRITE_INFO 0

ERR_CODE_e lib_infinite_buffer_create(infiniteBuffer_t *buffer, size_t bufLen)
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

ERR_CODE_e lib_infinite_buffer_write(infiniteBuffer_t *buffer, const char *data, size_t len)
{
    if (!buffer || !data || len == 0)
    {
        debug_printf("[WRITE ERROR] Invalid parameters\n");
        return FAILURE;
    }

    char *writePtr = (char *)atomic_load(&buffer->writePtr);
    char *readPtr = (char *)atomic_load(&buffer->readPtr);
    if (writePtr == readPtr)
    {

        if (atomic_load(&buffer->isEmpty))
        {
            // copy data to buffer
            ptrdiff_t freeLength = buffer->bufEnd - writePtr;
            if (freeLength < 0)
            {
                debug_printf("[WRITE ERROR] writePtr == readPtr(%p, %p), freeLength < 0(%td)\n", writePtr, readPtr, freeLength);
                return FAILURE;
            }
            // writePtr=readPtr, freeLength>len
            if ((size_t)freeLength >= len)
            {
                atomic_store(&buffer->isWriting, true);
                memcpy(writePtr, data, len);
                writePtr += len;
                atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
                atomic_store(&buffer->isFull, false);
                atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
                atomic_store(&buffer->isWriting, false);

                if (PRINTF_WRITE_INFO)
                    debug_printf("[WRITE] newDatalength=[%zu], writePtr=[%p]", len, writePtr);
                return SUCCESS;
            }
            else
            {
                ptrdiff_t freeLengthNext = readPtr - buffer->bufStart;
                if (freeLengthNext <= 0)
                {
                    debug_printf("[WRITE ERROR] writePtr == readPtr(%p, %p), freeLengthNext <= 0(%td)\n", writePtr, readPtr, freeLengthNext);
                    return FAILURE;
                }
                else
                {
                    if ((size_t)freeLengthNext > len)
                    {
                        atomic_store(&buffer->isWriting, true);
                        atomic_store(&buffer->waterMark, (atomic_uintptr_t)writePtr);
                        memcpy(buffer->bufStart, data, len);
                        writePtr = buffer->bufStart + len;
                        atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
                        atomic_store(&buffer->isFull, false);
                        atomic_store(&buffer->isWriting, false);
                        if (PRINTF_WRITE_INFO)
                            debug_printf("[WRITE] newDatalength=[%zu], writePtr=[%p]", len, writePtr);
                        return SUCCESS;
                    }
                    else if ((size_t)freeLengthNext == len)
                    {
                        atomic_store(&buffer->isWriting, true);
                        atomic_store(&buffer->waterMark, (atomic_uintptr_t)writePtr);
                        memcpy(buffer->bufStart, data, len);
                        writePtr = buffer->bufStart + len;
                        atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
                        atomic_store(&buffer->isFull, true);
                        atomic_store(&buffer->isWriting, false);
                        if (PRINTF_WRITE_INFO)
                            debug_printf("[WRITE] newDatalength=[%zu], writePtr=[%p], buffer is full", len, writePtr);
                        return SUCCESS;
                    }
                    else
                    {
                        debug_printf("[WRITE ERROR] writePtr == readPtr(%p, %p), freeLengthNext < len(%zu, %zu),Free is not enough space.", writePtr, readPtr, freeLengthNext, len);
                        return FAILURE;
                    }
                }
            }
        }
        else
        {
            debug_printf("[WRITE ERROR] writePtr == readPtr(%p, %p), isEmpty = false\n", writePtr, readPtr);
            return FAILURE;
        }
    }
    else if (writePtr > readPtr)
    {
        ptrdiff_t freeLength = buffer->bufEnd - writePtr;
        if (freeLength <= 0)
        {
            debug_printf("[WRITE ERROR] writePtr > readPtr(%p,%p), freeLength <= 0(%td)\n", writePtr, readPtr, freeLength);
            return FAILURE;
        }
        if ((size_t)freeLength >= len)
        {
            // copy data to buffer
            atomic_store(&buffer->isWriting, true);
            if (PRINTF_WRITE_INFO)
                debug_printf("[WRITE] writePtr > readPtr(%p,%p), freeLength >= len(%zu, %zu)\n", writePtr, readPtr, freeLength, len);
            memcpy(writePtr, data, len);
            writePtr += len;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, false);
            atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
            atomic_store(&buffer->isWriting, false);
            if (PRINTF_WRITE_INFO)
                debug_printf("[WRITE] newDatalength=[%zu], writePtr=[%p]", len, writePtr);
            return SUCCESS;
        }
        else
        {
            ptrdiff_t freeLengthNext = readPtr - buffer->bufStart;
            if (freeLengthNext < 0)
            {
                debug_printf("[WRITE ERROR] writePtr > readPtr(%p,%p), freeLengthNext < 0(%td)\n", writePtr, readPtr, freeLengthNext);
                return FAILURE;
            }

            if ((size_t)freeLengthNext > len)
            {
                // copy data to buffer
                atomic_store(&buffer->isWriting, true);
                atomic_store(&buffer->waterMark, (atomic_uintptr_t)writePtr);
                memcpy(buffer->bufStart, data, len);
                atomic_store(&buffer->isFull, false);
                writePtr = buffer->bufStart + len;
                atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
                atomic_store(&buffer->isWriting, false);
                if (PRINTF_WRITE_INFO)
                    debug_printf("[WRITE] newDatalength=[%zu], writePtr=[%p]", len, writePtr);
                return SUCCESS;
            }
            else if ((size_t)freeLengthNext == len)
            {
                // copy data to buffer
                atomic_store(&buffer->isWriting, true);
                memcpy(buffer->bufStart, data, len);
                writePtr = buffer->bufStart;
                atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
                atomic_store(&buffer->waterMark, (atomic_uintptr_t)buffer->bufEnd);
                atomic_store(&buffer->isFull, true);
                atomic_store(&buffer->isWriting, false);
                if (PRINTF_WRITE_INFO)
                    debug_printf("[WRITE] newDatalength=[%zu], writePtr=[%p], buffer is full", len, writePtr);
                return SUCCESS;
            }
            else
            {
                debug_printf("[WRITE ERROR] writePtr > readPtr(%p,%p), freeLength < len(%zu, %zu),Free is not enough space.", writePtr, readPtr, freeLength, len);
                return FAILURE;
            }
        }
    }
    else if (writePtr < readPtr)
    {
        ptrdiff_t freeLength = readPtr - writePtr;
        if (freeLength <= 0)
        {
            debug_printf("[WRITE ERROR] writePtr < readPtr(%p, %p), freeLength <= 0(%td)\n", writePtr, readPtr, freeLength);
            return FAILURE;
        }
        if ((size_t)freeLength > len)
        {
            // copy data to buffer
            atomic_store(&buffer->isWriting, true);
            memcpy(writePtr, data, len);
            writePtr += len;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, false);
            atomic_store(&buffer->isWriting, false);

            if (PRINTF_WRITE_INFO)
                debug_printf("[WRITE] writePtr < readPtr(%p, %p), freeLength > len(%zu, %zu)\n", writePtr, readPtr, freeLength, len);
            return SUCCESS;
        }
        else if ((size_t)freeLength == len)
        {
            // copy data to buffer
            atomic_store(&buffer->isWriting, true);
            memcpy(writePtr, data, len);
            writePtr += len;
            atomic_store(&buffer->writePtr, (atomic_uintptr_t)writePtr);
            atomic_store(&buffer->isFull, true);
            atomic_store(&buffer->isWriting, false);
            if (PRINTF_WRITE_INFO)
                debug_printf("[WRITE] writePtr < readPtr(%p, %p), freeLength == len(%zu, %zu),buffer is full\n", writePtr, readPtr, freeLength, len);
            return SUCCESS;
        }
        else
        {
            debug_printf("[WRITE ERROR] writePtr < readPtr(%p, %p), freeLength < len(%zu, %zu),Free is not enough space.\n", writePtr, readPtr, freeLength, len);
            return FAILURE;
        }
    }
}

ERR_CODE_e lib_infinite_buffer_write_wait(infiniteBuffer_t *buffer, const char *data, size_t len)
{
    ERR_CODE_e ret;
    bool isWriteSuccess = false;
    if (!buffer || !data || len == 0)
    {
        debug_printf("[WRITE ERROR] Invalid parameters\n");
        return FAILURE;
    }
    while (1)
    {
        ret = lib_infinite_buffer_write(buffer, data, len);
        if (ret == SUCCESS)
        {
            isWriteSuccess = true;
            break;
        }
        else
        {
            usleep(1);
        }
    }
    return SUCCESS;
}
int32_t lib_infinite_buffer_read(infiniteBuffer_t *buffer, char *data, size_t len)
{
    if (!buffer || !data || len == 0)
    {
        debug_printf("[READ ERROR] Invalid parameters\n");
        return -1;
    }

    if (atomic_load(&buffer->isReading))
    {
        debug_printf("[READ ERROR] buffer is reading\n");
        return -1;
    }

    atomic_store(&buffer->isReading, true);

    char *readPtr = (char *)atomic_load(&buffer->readPtr);
    char *writePtr = (char *)atomic_load(&buffer->writePtr);
    char *waterMark = (char *)atomic_load(&buffer->waterMark);

    if (writePtr == readPtr)
    {
        if (atomic_load(&buffer->isFull))
        {
            ptrdiff_t readableLength = waterMark - readPtr;
            if (readableLength <= 0)
            {
                debug_printf("[READ ERROR] writePtr == readPtr(%p, %p), readableLength < 0(%td)\n", writePtr, readPtr, readableLength);
                atomic_store(&buffer->isReading, false);
                return -1;
            }
            if ((size_t)readableLength >= len)
            {
                memcpy(data, readPtr, len);
                readPtr += len;
                atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
                atomic_store(&buffer->isEmpty, false);
                if (PRINTF_READ_INFO)
                    debug_printf("[READ] readableLength>=len(%zu, %zu)", readableLength, len);
                atomic_store(&buffer->isReading, false);
                return len;
            }
            else if ((size_t)readableLength < len)
            {
                memcpy(data, readPtr, (size_t)readableLength);
                readPtr = buffer->bufStart;
                atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
                atomic_store(&buffer->isEmpty, false);
                atomic_store(&buffer->isReading, false);
                debug_printf("[READ ] writePtr == readPtr(%p, %p), readableLength < len(%zu, %zu), wait for the remaining data.\n", writePtr, readPtr, readableLength, len);
                return (size_t)readableLength;
            }
        }
        else
        {
            atomic_store(&buffer->isEmpty, true);
            //debug_printf("[READ ERROR] writePtr == readPtr(%p, %p), set isEmpty = true\n", writePtr, readPtr);
            atomic_store(&buffer->isReading, false);

            return -1;
        }
    }
    else if (writePtr > readPtr)
    {
        ptrdiff_t readableLength = writePtr - readPtr;
        if (readableLength <= 0)
        {
            debug_printf("[READ ERROR] writePtr > readPtr(%p, %p), readableLength <= 0(%td)\n", writePtr, readPtr, readableLength);
            atomic_store(&buffer->isReading, false);
            return -1;
        }

        if ((size_t)readableLength > len)
        {
            memcpy(data, readPtr, len);
            readPtr += len;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, false);
            if (PRINTF_READ_INFO)
                debug_printf("[READ] readDataLength=[%zu], readPtr=[%p]\n", len, readPtr);
            atomic_store(&buffer->isReading, false);
            return len;
        }
        else if ((size_t)readableLength == len)
        {
            memcpy(data, readPtr, len);
            readPtr += len;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, true);
            if (PRINTF_READ_INFO)
                debug_printf("[READ] readDataLength=[%zu], readPtr=[%p]\n", len, readPtr);
            atomic_store(&buffer->isReading, false);

            return len;
        }
        else if ((size_t)readableLength < len)
        {
            memcpy(data, readPtr, (size_t)readableLength);
            readPtr += (size_t)readableLength;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, true);
            atomic_store(&buffer->isReading, false);
            if (PRINTF_READ_INFO)
                debug_printf("[READ] writePtr > readPtr(%p, %p), readableLength < len(%td, %zu).\n", writePtr, readPtr, readableLength, len);

            return (size_t)readableLength;
        }
    }
    else if (writePtr < readPtr)
    {
        ptrdiff_t readableLength = waterMark - readPtr;

        if (readableLength <= 0)
        {
            debug_printf("[READ ERROR] writePtr < readPtr(%p, %p), readableLength <= 0(%td)\n", writePtr, readPtr, readableLength);
            readPtr = buffer->bufStart;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isReading, false);
            return -1;
        }
        if ((size_t)readableLength > len)
        {
            memcpy(data, readPtr, len);
            readPtr += len;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, false);
            atomic_store(&buffer->isReading, false);
            return len;
        }
        else if ((size_t)readableLength == len)
        {
            if (PRINTF_READ_INFO)
                debug_printf("[READ] writePtr < readPtr(%p, %p), readableLength == len(%td, %zu)\n", writePtr, readPtr, readableLength, len);
            memcpy(data, readPtr, len);
            readPtr += len;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, false);
            if (PRINTF_READ_INFO)
                debug_printf("[READ] readDataLength=[%td], readPtr=[%p]\n", len, readPtr);
            atomic_store(&buffer->isReading, false);

            return len;
        }
        else if ((size_t)readableLength < len)
        {
            memcpy(data, readPtr, (size_t)readableLength);
            readPtr = buffer->bufStart;
            atomic_store(&buffer->readPtr, (atomic_uintptr_t)readPtr);
            atomic_store(&buffer->isEmpty, false);
            atomic_store(&buffer->isReading, false);
            if (PRINTF_READ_INFO)
                debug_printf("[READ] writePtr < readPtr(%p, %p), readableLength < len(%td, %zu),wait for the remaining data.\n", writePtr, readPtr, readableLength, len);
            return (size_t)readableLength;
        }
    }

    return SUCCESS;
}

int32_t lib_infinite_buffer_read_wait(infiniteBuffer_t *buffer, char *data, size_t len)
{
    ERR_CODE_e ret;
    bool isReadSuccess = false;
    size_t readBytesTotal = 0;
    size_t expectedReadBytes = len;
    if (!buffer || !data || len == 0)
    {
        debug_printf("[READ ERROR] Invalid parameters\n");
        return -1;
    }
    while (!isReadSuccess)
    {
        ret = lib_infinite_buffer_read(buffer, data, expectedReadBytes - readBytesTotal);
        if (ret == -1)
        {
            isReadSuccess = false;
            usleep(1);
            continue;
        }
        else
        {
            readBytesTotal += ret;
            if (readBytesTotal == expectedReadBytes)
            {
                isReadSuccess = true;
                break;
            }
        }
    }
    return len;
}

ERR_CODE_e lib_infinite_buffer_peek(infiniteBuffer_t *buffer, char *data, size_t len)
{
    return SUCCESS;
}

bool lib_infinite_buffer_isEmpty(infiniteBuffer_t *buffer)
{
    if (atomic_load(&buffer->isEmpty))
    {
        return true;
    }
    else
    {
        return false;
    }
    return SUCCESS;
}

ERR_CODE_e lib_infinite_buffer_reads_set_exclusive(infiniteBuffer_t *buffer)
{
    if (!buffer)
    {
        debug_printf("[EXCLUSIVE ERROR] Invalid buffer pointer\n");
        return FAILURE;
    }

    // Check if the buffer is already in exclusive mode
    if (atomic_load(&buffer->isExclusive))
    {
        debug_printf("[EXCLUSIVE ERROR] Buffer is already in exclusive mode\n");
        return FAILURE;
    }

    atomic_store(&buffer->isExclusive, true);
    return SUCCESS;
}

ERR_CODE_e lib_infinite_buffer_reads_unset_exclusive(infiniteBuffer_t *buffer)
{
    if (!buffer)
    {
        debug_printf("[EXCLUSIVE ERROR] Invalid buffer pointer\n");
        return FAILURE;
    }

    // Check if the buffer is not in exclusive mode
    if (!atomic_load(&buffer->isExclusive))
    {
        debug_printf("[EXCLUSIVE ERROR] Buffer is not in exclusive mode\n");
        return FAILURE;
    }

    atomic_store(&buffer->isExclusive, false);
    return SUCCESS;
}