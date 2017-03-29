#include "NotificationsWindow.h"
#include <pebble.h>
#include <pebble_fonts.h>
#include "ActionsMenu.h"
#include "tertiary_text.h"
#include "BackgroundLighterLayer.h"
#include "NotificationStorage.h"
#include "../NotificationCenter.h"
#include "../MainMenuWindow.h"
#include "UI.h"
#include "Buttons.h"
#include "Comm.h"
#include "Gestures.h"

uint32_t elapsedTime = 0;
bool appIdle = true;
bool vibrating = false;
bool lightOn = false;
bool autoSwitch = false;

uint16_t periodicVibrationPeriod = 0;

static Window* notifyWindow;

#ifdef PBL_COLOR
uint8_t* bitmapReceivingBuffer = NULL;
uint16_t bitmapReceivingBufferHead;
GBitmap* notificationBitmap = NULL;
#endif

#ifdef PBL_MICROPHONE
static DictationSession* dictationSession = NULL;
#endif

uint8_t pickedNotification = 0;

bool busy;
bool appIdle;

void nw_switch_to_notification(uint8_t index)
{
    pickedNotification = index;
    nw_ui_refresh_notification();
    nw_ui_scroll_to_notification_start();

#ifdef  PBL_COLOR
    if (notificationBitmap != NULL)
    {
        gbitmap_destroy(notificationBitmap);
        notificationBitmap = NULL;
        nw_ui_set_notification_image(NULL);
    }

    Notification* newNotification = nw_get_displayed_notification();
    if (newNotification != NULL && newNotification->imageSize != 0)
    {

        if (bitmapReceivingBuffer != NULL)
            free(bitmapReceivingBuffer);
        bitmapReceivingBuffer = malloc(newNotification->imageSize);
        bitmapReceivingBufferHead = 0;

        //Request image for currently selected notification

        DictionaryIterator *iterator;
        app_message_outbox_begin(&iterator);
        dict_write_uint8(iterator, 0, 5);
        dict_write_uint8(iterator, 1, 0);
        dict_write_int32(iterator, 2, newNotification->id);
        app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
        app_message_outbox_send();

    }
    #endif
}

Notification* nw_get_displayed_notification()
{
    return notificationData[pickedNotification];
}

void nw_fix_picked_notification()
{
    if (pickedNotification >= numOfNotifications && pickedNotification > 0)
        nw_switch_to_notification(numOfNotifications - 1);
}

bool nw_remove_notification_with_id(int32_t id, bool closeAutomatically)
{
    int8_t index = find_notification_index(id);
    if (index < 0)
        return false;

    nw_remove_notification((uint8_t) index, closeAutomatically);
    return true;
}

void nw_remove_notification(uint8_t index, bool closeAutomatically)
{
    if (numOfNotifications <= 1 && closeAutomatically)
    {
        tertiary_text_window_close();
        window_stack_pop(true);
        return;
    }

    remove_notification_from_storage(index);

    bool differentNotification = pickedNotification == index;

    if (pickedNotification >= index && pickedNotification > 0)
    {
        pickedNotification--;
    }

    if (differentNotification)
    {
        actions_menu_hide();
        tertiary_text_window_close();
        nw_switch_to_notification(pickedNotification);
    }
}

void nw_set_busy_state(bool newState)
{
    busy = newState;
    nw_ui_set_busy_indicator(newState);
}


#ifdef PBL_MICROPHONE
static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context)
    {
        if (status == DictationSessionStatusSuccess)
        {
            nw_send_reply_text(transcription);
        }
    }

    void nw_start_dictation()
    {
        nw_set_busy_state(false);

        if (dictationSession == NULL)
            dictationSession = dictation_session_create(400, dictation_session_callback, NULL);

        dictation_session_start(dictationSession);
    }
#endif

static void vibration_stopped(void* data)
{
    vibrating = false;
}

void nw_vibrate(VibePattern* vibePattern, uint16_t totalLength)
{
    vibes_cancel();
    vibes_enqueue_custom_pattern(*vibePattern);

    vibrating = true;
    app_timer_register(totalLength, vibration_stopped, NULL);

}

static void bt_handler(bool connected)
{
    if (!connected)
    {
        static char text[] = "Bluetooth Disconnected\0\0Your pebble has been disconnected from the phone.";
        static uint8_t textSize = sizeof(text) / sizeof(char);

        Notification* notification = add_notification(textSize - 1, 0);
        notification->id = SPECIAL_NOTIFICATION_BT_DISCONNECTED;
        notification->inList = false;
        notification->scrollToEnd = false;
        notification->showMenuOnSelectPress = false;
        notification->showMenuOnSelectHold = false;
        notification->shakeAction = 0;
        notification->numOfActionsInDefaultMenu = 0;
        notification->subtitleStart = 9999;
        notification->bodyStart = 24;
        notification->fontTitle = 7;
        notification->fontSubtitle = 4;
        notification->fontBody = 4;
        notification->onlyDismissable = true;
        notification->currentTextLength = textSize - 1;
        memcpy(notification->text, text, textSize);

        #ifdef PBL_COLOR
            notification->notificationColor = GColorBlack;
        #endif

        nw_switch_to_notification(numOfNotifications - 1);
        vibes_double_pulse();
    }
    else
    {
        nw_remove_notification_with_id(SPECIAL_NOTIFICATION_BT_DISCONNECTED, true);
    }
}

void accelerometer_shake(AccelAxisType axis, int32_t direction)
{
    if (vibrating) //Vibration seems to generate a lot of false positives
        return;

    if (config_gestures && actions_menu_is_displayed())
    {
        nw_send_action_menu_result(actions_menu_get_selected_index());
        return;
    }

    Notification* notification = nw_get_displayed_notification();
    if (notification == NULL)
        return;

    uint8_t shakeAction = notification->shakeAction;

    if (shakeAction == 0)
    {
        return;
    }
    else if (shakeAction == 60)
    {
        appIdle = false;
        return;
    }
    else if (shakeAction == 61)
    {
        appIdle = false;
        autoSwitch = true;
        return;
    }
    else if (shakeAction == 62)
    {
        nw_remove_notification(pickedNotification, true);
        return;
    }
    else if (shakeAction == 2)
    {
        actions_menu_set_number_of_items(notification->numOfActionsInDefaultMenu);
        actions_menu_reset_text();
        actions_menu_show();
    }

    nw_send_select_action(notification->id, 2);
}


static void second_tick(void)
{
    elapsedTime++;

    if (appIdle && config_timeout > 0 && config_timeout < elapsedTime && main_noMenu)
    {
        window_stack_pop(true);
        return;
    }

    if (lightOn && elapsedTime >= config_lightTimeout)
    {
        lightOn = false;
        light_enable(false);
    }

    if (periodicVibrationPeriod > 0 &&
        appIdle &&
        elapsedTime > 0 && elapsedTime % periodicVibrationPeriod == 0 &&
        !nw_get_displayed_notification()->inList &&
        canVibrate() &&
        (config_periodicTimeout == 0 || elapsedTime < config_periodicTimeout))
    {
        VibePattern pat = {
                .durations = config_periodicVibrationPattern,
                .num_segments = config_periodicVibrationPatternSize / 2,
        };

        nw_vibrate(&pat, config_periodicVibrationTotalDuration);
    }

    nw_ui_update_statusbar_clock();
}

static void window_appears(Window *window)
{
    setCurWindow(1);
}

static void window_load(Window *window)
{
    nw_ui_load(window);

    actions_menu_init();
    actions_menu_attach(window_get_root_layer(window));

    tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) second_tick);
    if (config_disconnectedNotification)
        bluetooth_connection_service_subscribe(bt_handler);

    numOfNotifications = 0;

    for (int i = 0; i < NOTIFICATION_SLOTS; i++)
    {
        notificationData[i] = NULL;
    }

    nw_ui_refresh_notification();
    if (config_gestures)
        nw_gestures_init();
    else
        accel_tap_service_subscribe(accelerometer_shake);
}

static void window_unload(Window *window)
{
    numOfNotifications = 0;

    nw_ui_unload();
    actions_menu_deinit();

    #ifdef PBL_COLOR
        if (bitmapReceivingBuffer != NULL)
        {
            free(bitmapReceivingBuffer);
            bitmapReceivingBuffer = NULL;
        }

        if (notificationBitmap != NULL)
        {
            gbitmap_destroy(notificationBitmap);
            notificationBitmap = NULL;
        }
    #endif

    #ifdef PBL_MICROPHONE
    if (dictationSession != NULL)
            dictation_session_destroy(dictationSession);
    #endif

    accel_tap_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    tick_timer_service_unsubscribe();
    nw_gestures_deinit();
    window_destroy(window);

    if (main_noMenu && config_dontClose)
    {
        closeApp();
    }

    if (main_noMenu)
        closingMode = true;

    for (int i = 0; i < NOTIFICATION_SLOTS; i++)
    {
        destroy_notification(notificationData[i]);
    }
}

void notification_window_init(void)
{
    notifyWindow = window_create();

    window_set_window_handlers(notifyWindow, (WindowHandlers) {
            .appear = (WindowHandler)window_appears,
            .load = (WindowHandler) window_load,
            .unload = (WindowHandler) window_unload
    });


    window_set_click_config_provider(notifyWindow, (ClickConfigProvider) nw_buttonconfig);
    window_stack_push(notifyWindow, true);

    if (!config_dontClose)
        main_menu_close();
}
