//
// Created by Matej on 20.10.2015.
//

#ifndef NOTIFICATIONCENTER_NOTIFICATIONSTORAGE_H
#define NOTIFICATIONCENTER_NOTIFICATIONSTORAGE_H

#include <pebble.h>

#define NOTIFICATION_SLOTS 10
#define NOTIFICATION_MEMORY_STORAGE_SIZE 4000

typedef struct
{
    int32_t id;
    bool inList;
    bool scrollToEnd;
    bool onlyDismissable;
    bool showMenuOnSelectPress;
    bool showMenuOnSelectHold;
    uint8_t shakeAction;
    uint8_t numOfActionsInDefaultMenu;
    uint8_t fontTitle;
    uint8_t fontSubtitle;
    uint8_t fontBody;

    uint16_t subtitleStart;
    uint16_t bodyStart;
    uint16_t currentTextLength;
    uint16_t totalTextLength;
#ifdef PBL_COLOR
    GColor8 notificationColor;
    uint16_t imageSize;
#endif
    char* text;
} Notification;


extern Notification* notificationData[NOTIFICATION_SLOTS];
extern uint8_t numOfNotifications;

Notification* create_notification(uint16_t textLength);
Notification* find_notification(int32_t id);
Notification* add_notification(uint16_t textSize);
void remove_notification_from_storage(uint8_t id);
void destroy_notification(Notification* notification);


#endif //NOTIFICATIONCENTER_NOTIFICATIONSTORAGE_H
