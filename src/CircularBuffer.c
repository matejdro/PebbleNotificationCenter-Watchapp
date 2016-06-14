//
// Created by Matej on 18.12.2015.
//

#include "CircularBuffer.h"

void cb_clear(CircularBuffer* buffer)
{
    for (int i = 0; i < buffer->maxEntriesInBuffer; i++)
        buffer->loaded[i] = false;
}

CircularBuffer* cb_create(size_t singleEntrySize, uint8_t maxEntriesInBuffer)
{
    CircularBuffer* circularBuffer = (CircularBuffer*) malloc(sizeof(CircularBuffer));
    circularBuffer->data = malloc(singleEntrySize * maxEntriesInBuffer);
    circularBuffer->loaded = malloc(sizeof(bool) * maxEntriesInBuffer);
    circularBuffer->singleEntrySize = singleEntrySize;
    circularBuffer->maxEntriesInBuffer = maxEntriesInBuffer;
    circularBuffer->bufferCenterPos = 0;
    circularBuffer->centerIndex = 0;

    cb_clear(circularBuffer);

    return circularBuffer;
}

void cb_destroy(CircularBuffer* buffer)
{
    free(buffer->loaded);
    free(buffer->data);
    free(buffer);
}


void cb_shift(CircularBuffer* circularBuffer, uint16_t newIndex)
{
    int8_t clearIndex;

    int16_t diff = newIndex - circularBuffer->centerIndex;
    if (diff == 0)
        return;

    circularBuffer->centerIndex += diff;
    circularBuffer->bufferCenterPos += diff;

    if (diff > 0)
    {
        if (circularBuffer->bufferCenterPos >= circularBuffer->maxEntriesInBuffer)
            circularBuffer->bufferCenterPos -= circularBuffer->maxEntriesInBuffer;

        clearIndex = (int8_t) (circularBuffer->bufferCenterPos - circularBuffer->maxEntriesInBuffer / 2);
        if (clearIndex < 0)
            clearIndex += circularBuffer->maxEntriesInBuffer;
    }
    else
    {
        if (circularBuffer->bufferCenterPos < 0)
            circularBuffer->bufferCenterPos += circularBuffer->maxEntriesInBuffer;

        clearIndex = (int8_t) (circularBuffer->bufferCenterPos + circularBuffer->maxEntriesInBuffer / 2);
        if (clearIndex >= circularBuffer->maxEntriesInBuffer)
            clearIndex -= circularBuffer->maxEntriesInBuffer;
    }

    circularBuffer->loaded[clearIndex] = false;
}


static int8_t convertToArrayPos(CircularBuffer* buffer, uint16_t index)
{
    int16_t indexDiff = index - buffer->centerIndex;
    if (indexDiff >= buffer->maxEntriesInBuffer / 2 || indexDiff <= -buffer->maxEntriesInBuffer / 2)
        return -1;

    int8_t arrayPos = (int8_t) (buffer->bufferCenterPos + indexDiff);
    if (arrayPos < 0)
        arrayPos += buffer->maxEntriesInBuffer;
    if (arrayPos >= buffer->maxEntriesInBuffer)
        arrayPos -= buffer->maxEntriesInBuffer;

    return arrayPos;
}

void* cb_getEntry(CircularBuffer* buffer, uint16_t index)
{
    int8_t bufferPosition = convertToArrayPos(buffer, index);
    if (bufferPosition < 0)
        return NULL;

    if (!buffer->loaded[bufferPosition])
        return NULL;

    return (void*) &buffer->data[buffer->singleEntrySize * bufferPosition];

}

bool cb_isLoaded(CircularBuffer* buffer, uint16_t index)
{
    int8_t bufferPosition = convertToArrayPos(buffer, index);
    if (bufferPosition < 0)
        return false;

    return buffer->loaded[bufferPosition];
}

void* cb_getEntryForFilling(CircularBuffer* buffer, uint16_t index)
{
    int8_t bufferPosition = convertToArrayPos(buffer, index);
    if (bufferPosition < 0)
        return NULL;

    buffer->loaded[bufferPosition] = true;

    return (void*) &buffer->data[buffer->singleEntrySize * bufferPosition];
}

uint8_t cb_getNumOfLoadedSpacesDownFromCenter(CircularBuffer* buffer, uint16_t limit)
{
    uint8_t spaces = 0;
    for (int16_t i = buffer->centerIndex; i < buffer->centerIndex + buffer->maxEntriesInBuffer / 2; i++)
    {
        if (i >= limit)
            return buffer->maxEntriesInBuffer / 2; //Everything up to limit is loaded. Lets pretend everything beyond is already loaded.

        if (!cb_isLoaded(buffer, (uint8_t) i))
        {
            break;
        }

        spaces++;
    }

    return spaces;
}

uint8_t cb_getNumOfLoadedSpacesUpFromCenter(CircularBuffer* buffer)
{
    uint8_t spaces = 0;
    for (int16_t i = buffer->centerIndex; i > buffer->centerIndex - buffer->maxEntriesInBuffer / 2; i--)
    {
        if (i < 0)
            return buffer->maxEntriesInBuffer / 2; //Everything up to first entry is loaded. Lets pretend everything beyond is already loaded.

        if (!cb_isLoaded(buffer, (uint8_t) i))
        {
            break;
        }

        spaces++;
    }

    return spaces;

}

