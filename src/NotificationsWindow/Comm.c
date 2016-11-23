//
// Created by Matej on 20.10.2015.
//

#include <pebble.h>
#include "Comm.h"
#include "ActionsMenu.h"
#include "NotificationsWindow.h"
#include "../NotificationCenter.h"
#include "UI.h"
#include "NotificationStorage.h"

static int8_t pickedAction = -1;
static bool removeBusyOnSent = false;

void nw_send_action_menu_result(int action)
{
    pickedAction = -1;

    DictionaryIterator *iterator;
    AppMessageResult result = app_message_outbox_begin(&iterator);
    if (result != APP_MSG_OK)
    {
        pickedAction = action;
        return;
    }

    dict_write_uint8(iterator, 0, 4);
    dict_write_uint8(iterator, 1, 2);
    dict_write_uint8(iterator, 2, action);

    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
    app_message_outbox_send();

    nw_set_busy_state(true);
    actions_menu_hide();
}

void nw_send_reply_text(char* text)
{
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    dict_write_uint8(iterator, 0, 4);
    dict_write_uint8(iterator, 1, 3);
    dict_write_cstring(iterator, 2, text);
    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
    app_message_outbox_send();

    nw_set_busy_state(true);
}

void nw_send_list_notification_switch(int8_t change)
{
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    dict_write_uint8(iterator, 0, 2);
    dict_write_uint8(iterator, 1, 2);
    dict_write_int8(iterator, 2, change);
    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
    app_message_outbox_send();

    nw_set_busy_state(true);
}

void nw_send_select_action(int32_t notificationId, uint8_t actionType)
{
    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    dict_write_uint8(iterator, 0, 4);
    dict_write_uint8(iterator, 1, 0);

    dict_write_int32(iterator, 2, notificationId);
    dict_write_uint8(iterator, 3, actionType);

    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
    app_message_outbox_send();

    nw_set_busy_state(true);
}

static void nw_confirm_notification(int32_t notificationId)
{
    nw_set_busy_state(true);
    removeBusyOnSent = true;

    DictionaryIterator *iterator;
    app_message_outbox_begin(&iterator);
    dict_write_uint8(iterator, 0, 1);
    dict_write_uint8(iterator, 1, 0);
    dict_write_int32(iterator, 2, notificationId);
    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
    app_message_outbox_send();
}

static void received_message_new_notification(DictionaryIterator *received)
{
    int32_t id = dict_find(received, 2)->value->int32;

    uint8_t* configBytes = dict_find(received, 3)->value->data;

    uint8_t flags = configBytes[0];
    bool inList = (flags & 0x02) != 0;
    autoSwitch |= (flags & 0x04) != 0;

    uint16_t newPeriodicVibration = configBytes[1] << 8 | configBytes[2];
    if (newPeriodicVibration < periodicVibrationPeriod || periodicVibrationPeriod == 0)
        periodicVibrationPeriod = newPeriodicVibration;

    uint16_t textSize = configBytes[4] << 8 | configBytes[5];
    uint16_t iconSize = 0;
    #ifndef PBL_LOW_MEMORY
        Tuple* iconSizeTuple = dict_find(received, 5);
        if (iconSizeTuple != NULL)
            iconSize = iconSizeTuple->value->uint16;
    #endif

    uint8_t numOfVibrationBytes = configBytes[17];

    Notification* notification = find_notification(id);
    if (notification == NULL)
    {
        notification = add_notification(textSize, iconSize);

        bool blockVibration = false;
        int32_t oldId = dict_find(received, 4)->value->int32;
        if (oldId != 0 && find_notification(oldId) != NULL)
            blockVibration = true;

        if (!inList)
        {
            if (canVibrate() && !blockVibration)
            {
                bool shouldVibrate = false;

                uint16_t totalLength = 0;
                uint32_t segments[20];
                for (int i = 0; i < numOfVibrationBytes; i+= 2)
                {
                    segments[i / 2] = configBytes[18 +i] | (configBytes[19 +i] << 8);
                    totalLength += segments[i / 2];
                    if (i % 4 == 0 && segments[i / 2] > 0)
                        shouldVibrate = true;
                }
                if (shouldVibrate)
                {
                    VibePattern pat = {
                            .durations = segments,
                            .num_segments = numOfVibrationBytes / 2,
                    };

                    nw_vibrate(&pat, totalLength);
                }
            }

            if (config_lightScreen)
            {
                lightOn = true;
                light_enable(true);
            }

            appIdle = true;
            elapsedTime = 0;
        }
    }

    notification->id = id;
    notification->inList = inList;
    notification->scrollToEnd = (flags & 0x08) != 0;
    notification->showMenuOnSelectPress = (flags & 0x10) != 0;
    notification->showMenuOnSelectHold = (flags & 0x20) != 0;
    notification->shakeAction = configBytes[6];
    notification->numOfActionsInDefaultMenu = configBytes[3];
    notification->subtitleStart = configBytes[13] << 8 | configBytes[14];
    notification->bodyStart = configBytes[15] << 8 | configBytes[16];
    notification->fontTitle = configBytes[7];
    notification->fontSubtitle = configBytes[8];
    notification->fontBody = configBytes[9];
    notification->onlyDismissable = false;
    notification->currentTextLength = 0;
    notification->text[0] = 0;


#ifdef PBL_COLOR
    notification->notificationColor = (GColor8) {.argb = configBytes[10]};
    notification->imageSize = configBytes[11] << 8 | configBytes[12];
#endif

    nw_confirm_notification(notification->id);

    if (notification->inList)
    {
        for (int i = 0; i < numOfNotifications; i++)
        {
            Notification* entry = notificationData[i];
            if (entry->id == notification->id)
                continue;

            if (entry->inList)
            {
                nw_remove_notification(i, false);
                i--;
            }
        }
    }

    if (numOfNotifications == 1 || (autoSwitch && !actions_menu_is_displayed()))
        nw_switch_to_notification(numOfNotifications - 1);
    else
        nw_fix_picked_notification();

    nw_set_busy_state(false);
    autoSwitch = false;
}

static void received_message_dismiss(DictionaryIterator *received)
{
    int32_t id = dict_find(received, 2)->value->int32;
    bool close = !dict_find(received, 3)->value->uint8 == 1;

    nw_remove_notification_with_id(id, close);
    nw_set_busy_state(false);
}

static void received_message_more_text(DictionaryIterator *received)
{
    int32_t id = dict_find(received, 2)->value->int32;

    Notification* notification = find_notification(id);
    if (notification == NULL)
        return;

    uint16_t length = notification->totalTextLength -  notification->currentTextLength;
    if (length > 100)
        length = 100;

    memcpy(&notification->text[notification->currentTextLength], dict_find(received, 3)->value->data, length);

    notification->currentTextLength += length;
    notification->text[notification->currentTextLength] = 0;

    if (pickedNotification == numOfNotifications - 1)
    {
        nw_ui_refresh_notification();

        if (notification->scrollToEnd)
            nw_ui_scroll_to_notification_start();
    }
}

static void received_message_notification_list_items(DictionaryIterator *received)
{
    if (actions_menu_got_items(received))
        nw_set_busy_state(false);
}

#ifdef PBL_COLOR
static void received_message_image(DictionaryIterator* received)
{
    if (bitmapReceivingBuffer == NULL)
        return;

    Notification* curNotification = nw_get_displayed_notification();
    if (curNotification == NULL)
        return;

    uint8_t* buffer = dict_find(received, 2)->value->data;

    uint8_t curIndexFromPhone = buffer[0];
    if (bitmapReceivingBufferHead % 256 != curIndexFromPhone) //Simple checksum to make sure images do not collide
        return;

    uint16_t bufferSize = curNotification->imageSize - bitmapReceivingBufferHead;
    bool finished = true;

    uint16_t maxBufferSize = (uint16_t) (appmessage_max_size - 3 * 7 - 2 - 1 - 1); //Substract overhead from maximum message size.


    if (bufferSize > maxBufferSize)
    {
        finished = false;
        bufferSize = maxBufferSize;
    }

    memcpy(&bitmapReceivingBuffer[bitmapReceivingBufferHead], &buffer[1], bufferSize);
    bitmapReceivingBufferHead += bufferSize;

    if (finished)
    {
        if (notificationBitmap != NULL)
            gbitmap_destroy(notificationBitmap);

        notificationBitmap = gbitmap_create_from_png_data(bitmapReceivingBuffer, curNotification->imageSize);
        nw_ui_set_notification_image(notificationBitmap);

        free(bitmapReceivingBuffer);
        bitmapReceivingBuffer = NULL;
    }
}
#endif

#ifndef PBL_LOW_MEMORY
    void received_message_icon(DictionaryIterator* received)
    {
        int32_t notificationId = dict_find(received, 2)->value->int32;

        Notification* notification = find_notification(notificationId);
        if (notification == NULL)
            return;

        if (notification->notificationIcon != NULL)
            gbitmap_destroy(notification->notificationIcon);

        if (notification->notificationIconData != NULL)
        {
            memcpy(notification->notificationIconData, dict_find(received, 3)->value->data, notification->iconSize);
            notification->notificationIcon = gbitmap_create_from_png_data(notification->notificationIconData, notification->iconSize);
        }
        else
            notification->notificationIcon = NULL;

    }
#endif

void nw_received_data_callback(uint8_t module, uint8_t id, DictionaryIterator* received)
{
    if (module == 0 && id == 1)
    {
        nw_set_busy_state(false);
    }
    else if (module == 1)
    {
        if (id == 0)
            received_message_new_notification(received);
        else if (id == 1)
            received_message_more_text(received);
        #ifndef PBL_LOW_MEMORY
            else if (id == 2)
                received_message_icon(received);
        #endif
    }
    else if (module == 3 && id == 0)
    {
        received_message_dismiss(received);
    }
    else if (module == 4 && id == 0)
    {
        received_message_notification_list_items(received);
    }
#ifdef PBL_COLOR
    else if (module == 5 && id == 0)
    {
        received_message_image(received);
    }
#endif
#ifdef PBL_MICROPHONE
    else if (module == 4 && id == 1)
    {
        nw_start_dictation();
    }
#endif
}

void nw_data_sent_callback(void)
{
    if (pickedAction > -1)
        nw_send_action_menu_result(pickedAction);
    else if (removeBusyOnSent)
    {
        nw_set_busy_state(false);
        removeBusyOnSent = false;
    }
}
