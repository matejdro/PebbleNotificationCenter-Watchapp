//
// Created by Matej on 20.10.2015.
//

#include <pebble.h>

#include "NotificationStorage.h"

Notification* notificationData[NOTIFICATION_SLOTS];
static uint16_t freeNotificationMemory = NOTIFICATION_MEMORY_STORAGE_SIZE;
uint8_t numOfNotifications = 0;

static Notification* create_notification(uint16_t textLength, uint16_t iconSize)
{
    Notification* notification = malloc(sizeof(Notification));
    notification->totalTextLength = textLength;
    textLength++; //Reserve one byte for null character

    notification->text = malloc(sizeof(char) * textLength);

    #ifndef PBL_LOW_MEMORY
        if (iconSize > 0)
        {
            notification->notificationIconData = malloc(iconSize);
            notification->iconSize = iconSize;

            freeNotificationMemory -= iconSize;
        }
        else
            notification->notificationIconData = NULL;

        notification->notificationIcon = NULL;
    #endif

    freeNotificationMemory -= sizeof(Notification) + sizeof(char) * textLength;

    return notification;
}

void destroy_notification(Notification* notification)
{
    if (notification == NULL)
        return;

    freeNotificationMemory += sizeof(Notification) + sizeof(char) * (notification->totalTextLength + 1);

    #ifndef PBL_LOW_MEMORY
        freeNotificationMemory += notification->iconSize;

        gbitmap_destroy(notification->notificationIcon);
        free(notification->notificationIconData);
    #endif

    free(notification->text);
    free(notification);
}

void remove_notification_from_storage(uint8_t id)
{
    if (numOfNotifications > 0)
        numOfNotifications--;

    destroy_notification(notificationData[id]);

    for (int i = id; i < numOfNotifications; i++)
    {
        notificationData[i] = notificationData[i + 1];
    }

    notificationData[numOfNotifications] = NULL;
}

Notification* add_notification(uint16_t textSize, uint16_t iconSize)
{
    if (numOfNotifications >= NOTIFICATION_SLOTS)
        remove_notification_from_storage(0);

    uint16_t totalSize = sizeof(Notification) + sizeof(char) * (textSize + 1) + iconSize;
    while (freeNotificationMemory < totalSize)
        remove_notification_from_storage(0);

    uint8_t position = numOfNotifications;
    numOfNotifications++;

    Notification* notification = create_notification(textSize, iconSize);
    notificationData[position] = notification;

    return notification;
}

Notification* find_notification(int32_t id)
{
    for (int i = 0; i < numOfNotifications; i++)
    {
        Notification* notification = notificationData[i];
        if (notification != NULL && notification->id == id)
            return notificationData[i];
    }

    return NULL;
}

int8_t find_notification_index(int32_t id)
{
    for (int i = 0; i < numOfNotifications; i++)
    {
        Notification* notification = notificationData[i];
        if (notification != NULL && notification->id == id)
            return i;
    }

    return -1;
}

