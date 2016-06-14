//
// Created by Matej on 18.12.2015.
//

#ifndef DIALER2_CIRCULARBUFFER_H
#define DIALER2_CIRCULARBUFFER_H

#include <pebble.h>

typedef struct
{
    bool* loaded;
    char* data;
    int8_t bufferCenterPos;
    uint16_t centerIndex;

    uint8_t maxEntriesInBuffer;
    size_t singleEntrySize;

} CircularBuffer;

CircularBuffer* cb_create(size_t singleEntrySize,  uint8_t maxEntriesInBuffer);
void* cb_getEntry(CircularBuffer* buffer, uint16_t index);
void* cb_getEntryForFilling(CircularBuffer* buffer, uint16_t index);
bool cb_isLoaded(CircularBuffer* buffer, uint16_t index);
void cb_destroy(CircularBuffer* buffer);
void cb_shift(CircularBuffer* buffer, uint16_t newIndex);
uint8_t cb_getNumOfLoadedSpacesDownFromCenter(CircularBuffer* buffer, uint16_t limit);
uint8_t cb_getNumOfLoadedSpacesUpFromCenter(CircularBuffer* buffer);
void cb_clear(CircularBuffer* buffer);


#endif //DIALER2_CIRCULARBUFFER_H
