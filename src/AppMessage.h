#include <pebble.h>

AppMessageResult reliable_app_message_outbox_begin(DictionaryIterator ** iterator);
void reliable_app_message_write_uint8(DictionaryIterator * iter, const uint32_t key, const uint8_t value);
void reliable_app_message_write_uint16(DictionaryIterator * iter, const uint32_t key, const uint16_t value);
void reliable_app_message_write_uint32(DictionaryIterator * iter, const uint32_t key, const uint32_t value);
void reliable_app_message_write_int8(DictionaryIterator * iter, const uint32_t key, const int8_t value);
void reliable_app_message_write_int16(DictionaryIterator * iter, const uint32_t key, const int16_t value);
void reliable_app_message_write_int32(DictionaryIterator * iter, const uint32_t key, const int32_t value);
void reliable_app_message_outbox_send();

typedef struct {
    TupleType type;
    int32_t sint;
    uint64_t uint;
} StoredAppMessage;