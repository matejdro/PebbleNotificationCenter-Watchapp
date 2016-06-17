//
// Created by Matej on 20.10.2015.
//

#include "Buttons.h"
#include "pebble.h"
#include "ActionsMenu.h"
#include "NotificationStorage.h"
#include "NotificationsWindow.h"
#include "UI.h"
#include "Comm.h"

static bool upPressed = false;
static bool downPressed = false;
static uint8_t skippedUpPresses = 0;
static uint8_t skippedDownPresses = 0;

static void button_back_single(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
    {
        actions_menu_hide();
    }
    else
        window_stack_pop(true);
}

static void button_center_single(ClickRecognizerRef recognizer, void* context)
{
    appIdle = false;

    Notification* curNotification = nw_get_displayed_notification();
    if (curNotification == NULL)
        return;

    if (curNotification->onlyDismissable)
    {
        nw_remove_notification(pickedNotification, true);
        return;
    }

    if (actions_menu_is_displayed())
    {
        nw_send_action_menu_result(actions_menu_get_selected_index());
    }
    else
    {
        if (busy)
            return;

        if (curNotification->showMenuOnSelectPress)
        {
            actions_menu_set_number_of_items(curNotification->numOfActionsInDefaultMenu);
            actions_menu_reset_text();
            actions_menu_show();
        }

        nw_send_select_action(curNotification->id, 0);
    }
}

static void button_center_hold(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
        return;

    Notification* curNotification = nw_get_displayed_notification();
    if (curNotification == NULL)
        return;

    if (curNotification->onlyDismissable)
    {
        nw_remove_notification(pickedNotification, true);
        return;
    }

    if (busy)
        return;

    if (curNotification->showMenuOnSelectHold)
    {
        actions_menu_set_number_of_items(curNotification->numOfActionsInDefaultMenu);
        actions_menu_reset_text();
        actions_menu_show();
    }

    nw_send_select_action(curNotification->id, 1);
}

static void button_up_raw_pressed(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
    {
        actions_menu_move_up();
    }
    else
    {
        nw_ui_scroll_notification(false);
    }

    appIdle = false;
    upPressed = true;
    skippedUpPresses = 0;
}

static void button_down_raw_pressed(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
    {
        actions_menu_move_down();
    }
    else
    {
        nw_ui_scroll_notification(true);
    }

    appIdle = false;
    downPressed = true;
    skippedDownPresses = 0;
}

static void button_up_raw_released(ClickRecognizerRef recognizer, void* context)
{
    upPressed = false;
}

static void button_down_raw_released(ClickRecognizerRef recognizer, void* context)
{
    downPressed = false;
}

static void button_up_click_proxy(ClickRecognizerRef recognizer, void* context)
{
    if (upPressed)
    {
        if (actions_menu_is_displayed())
        {
            actions_menu_move_up();
            return;
        }

        //Scroll layer can't handle being pressed every 50ms, so we only use 1/2 of the presses.
        if (skippedUpPresses == 1 )
        {
            nw_ui_scroll_notification(false);
            skippedUpPresses = 0;
        }
        else
        {
            skippedUpPresses = 1;
        }
    }
}

static void button_down_click_proxy(ClickRecognizerRef recognizer, void* context)
{
    if (downPressed)
    {
        if (actions_menu_is_displayed())
        {
            actions_menu_move_down();
            return;
        }


        //Scroll layer can't handle being pressed every 50ms, so we only use 1/2 of the presses.
        if (skippedDownPresses == 1 )
        {
            nw_ui_scroll_notification(true);
        }
        else
        {
            skippedDownPresses = 1;
        }
    }
}

static void button_up_double(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
        return;

    if (pickedNotification == 0)
    {
        Notification* notification = nw_get_displayed_notification();
        if (notification->inList && !busy)
        {
            nw_send_list_notification_switch(-1);
            return;
        }
    }

    if (numOfNotifications == 1)
    {
        nw_ui_scroll_notification(false);
        return;
    }

    if (pickedNotification == 0)
        nw_switch_to_notification(numOfNotifications - 1);
    else
        nw_switch_to_notification(pickedNotification - 1);
}

static void button_down_double(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
        return;

    if (pickedNotification == numOfNotifications - 1)
    {
        Notification* notification = nw_get_displayed_notification();
        if (notification->inList && !busy)
        {
            nw_send_list_notification_switch(1);
            return;
        }
    }

    if (numOfNotifications == 1)
    {
        nw_ui_scroll_notification(true);
        return;
    }

    if (pickedNotification == numOfNotifications - 1)
        nw_switch_to_notification(0);
    else
        nw_switch_to_notification(pickedNotification + 1);
}

void nw_buttonconfig(void* context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) button_center_single);
    window_single_click_subscribe(BUTTON_ID_BACK, (ClickHandler) button_back_single);

    window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 150, false, (ClickHandler) button_up_double);
    window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 150, false, (ClickHandler) button_down_double);

    window_raw_click_subscribe(BUTTON_ID_UP, (ClickHandler) button_up_raw_pressed, (ClickHandler) button_up_raw_released, NULL);
    window_raw_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) button_down_raw_pressed, (ClickHandler) button_down_raw_released, NULL);

    window_long_click_subscribe(BUTTON_ID_SELECT, 500, (ClickHandler) button_center_hold, NULL);

    window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, (ClickHandler) button_up_click_proxy);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, (ClickHandler) button_down_click_proxy);
}

void nw_simulate_button_down()
{
    downPressed = true;
    skippedDownPresses = 1;
    button_down_click_proxy(NULL, NULL);
    downPressed = false;

    light_enable_interaction();
}

void nw_simulate_button_up()
{
    upPressed = true;
    skippedUpPresses = 1;
    button_up_click_proxy(NULL, NULL);
    upPressed = false;

    light_enable_interaction();
}