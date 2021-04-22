#include "AppMessage.h"
#include <pebble.h>

static uint8_t pending_message_buffer[200];
static DictionaryIterator pending_message;
static AppTimer *pending_timer = NULL;

static void timer_callback(void *data);

static void send_with_retry() {
    if (app_message_outbox_send() == APP_MSG_OK)
    {
        return;
    }

    const int interval_ms = 500;
    app_timer_register(interval_ms, timer_callback, NULL);
}

static void cancel_timer()
{
    if (pending_timer != NULL)
    {
        app_timer_cancel(pending_timer);
        pending_timer = NULL;
    }
}

static void timer_callback(void *data)
{    
    DictionaryIterator *transmit_iterator;
    app_message_outbox_begin(&transmit_iterator);

    Tuple *tuple = dict_read_first(&pending_message);
    while (tuple != NULL)
    {
        switch (tuple->type)
        {
        case TUPLE_UINT:
            switch (tuple->length)
            {
            case 1:
                dict_write_uint8(transmit_iterator, tuple->key, tuple->value->uint8);
                break;
            case 2:
                dict_write_uint16(transmit_iterator, tuple->key, tuple->value->uint16);
                break;
            case 4:
                dict_write_uint32(transmit_iterator, tuple->key, tuple->value->uint32);
                break;
            }
            break;
        case TUPLE_INT:
            switch (tuple->length)
            {
            case 1:
                dict_write_int8(transmit_iterator, tuple->key, tuple->value->int8);
                break;
            case 2:
                dict_write_int16(transmit_iterator, tuple->key, tuple->value->int16);
                break;
            case 4:
                dict_write_int32(transmit_iterator, tuple->key, tuple->value->int32);
                break;
            }
            break;
        default:
            break;
        }

        tuple = dict_read_next(&pending_message);
    }

    send_with_retry();
}

AppMessageResult reliable_app_message_outbox_begin(DictionaryIterator **iterator)
{
    cancel_timer();
    dict_write_begin(&pending_message, pending_message_buffer, sizeof(pending_message_buffer));
    return app_message_outbox_begin(iterator);
}

void reliable_app_message_write_uint8(DictionaryIterator *iter, const uint32_t key, const uint8_t value)
{
    dict_write_uint8(iter, key, value);
    dict_write_uint8(&pending_message, key, value);
}

void reliable_app_message_write_uint16(DictionaryIterator *iter, const uint32_t key, const uint16_t value)
{
    dict_write_uint16(iter, key, value);
    dict_write_uint16(&pending_message, key, value);
}

void reliable_app_message_write_uint32(DictionaryIterator *iter, const uint32_t key, const uint32_t value)
{
    dict_write_uint32(iter, key, value);
    dict_write_uint32(&pending_message, key, value);
}

void reliable_app_message_write_int8(DictionaryIterator *iter, const uint32_t key, const int8_t value)
{
    dict_write_int8(iter, key, value);
    dict_write_int8(&pending_message, key, value);
}

void reliable_app_message_write_int16(DictionaryIterator *iter, const uint32_t key, const int16_t value)
{
    dict_write_int16(iter, key, value);
    dict_write_int16(&pending_message, key, value);
}

void reliable_app_message_write_int32(DictionaryIterator *iter, const uint32_t key, const int32_t value)
{
    dict_write_int32(iter, key, value);
    dict_write_int32(&pending_message, key, value);
}

void reliable_app_message_outbox_send()
{
    dict_write_end(&pending_message);
    send_with_retry();
}