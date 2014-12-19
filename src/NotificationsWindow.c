#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "MainMenuWindow.h"
#include "ActionsMenu.h"
#include "tertiary_text.h"

#define NOTIFICATION_SLOTS 5

typedef struct
{
	int32_t id;
	bool inList;
	bool scrollToEnd;
	bool showMenuOnSelectPress;
	bool showMenuOnSelectHold;
	uint8_t numOfActionsInDefaultMenu;
	char title[31];
	char subTitle[31];
	char text[851];

} Notification;


static uint32_t elapsedTime = 0;
static bool appIdle = true;
static bool vibrating = false;

static uint16_t periodicVibrationPeriod = 0;

static Window* notifyWindow;

static InverterLayer* inverterLayer;

static Layer* statusbar;
static TextLayer* statusClock;
static char clockText[9];

static Layer* circlesLayer;

static bool busy;
static GBitmap* busyIndicator;

static uint8_t numOfNotifications = 0;
static uint8_t pickedNotification = 0;
static int8_t pickedAction = -1;

static Notification notificationData[NOTIFICATION_SLOTS];
static uint8_t notificationPositions[NOTIFICATION_SLOTS];
static bool notificationDataUsed[NOTIFICATION_SLOTS];

static ScrollLayer* scroll;

static bool upPressed = false;
static bool downPressed = false;
static uint8_t skippedUpPresses = 0;
static uint8_t skippedDownPresses = 0;

static TextLayer* title;
static TextLayer* subTitle;
static TextLayer* text;

static void registerButtons(void* context);

static void refresh_notification(void)
{
	char* titleText;
	char* subtitleText;
	char* bodyText;

	Notification* notification;

	if (numOfNotifications < 1)
	{
		titleText = "No notifications";
		subtitleText = "";
		bodyText = "";
		notification = NULL;
	}
	else
	{
		notification = &notificationData[notificationPositions[pickedNotification]];
		titleText = notification->title;
		subtitleText = notification->subTitle;
		bodyText = notification->text;
	}

	text_layer_set_text(title, titleText);
	text_layer_set_text(subTitle, subtitleText);
	text_layer_set_text(text, bodyText);

	text_layer_set_size(title, GSize(144 - 4, 30000));
	text_layer_set_size(subTitle, GSize(144 - 4, 30000));
	text_layer_set_size(text, GSize(144 - 4, 30000));

	GSize titleSize = text_layer_get_content_size(title);
	GSize subtitleSize = text_layer_get_content_size(subTitle);
	GSize textSize = text_layer_get_content_size(text);


	titleSize.h += 3;
	subtitleSize.h += 3;
	textSize.h += 5;

	text_layer_set_size(title, titleSize);

	layer_set_frame((Layer*) subTitle, GRect(3, titleSize.h + 1, 144 - 6, subtitleSize.h));
	layer_set_frame((Layer*) text, GRect(3, titleSize.h + 1 + subtitleSize.h + 1, 144 - 6, textSize.h));

	int16_t verticalSize = titleSize.h + 1 + subtitleSize.h + 1 + textSize.h + 5;

	scroll_layer_set_content_size(scroll, GSize(144 - 4, verticalSize));

	layer_mark_dirty(circlesLayer);
}

static void scroll_to_notification_start(void)
{
	int16_t scrollTo = 0;

	Notification* notification = &notificationData[notificationPositions[pickedNotification]];;
	if (notification != NULL && notification->scrollToEnd)
		scrollTo = -scroll_layer_get_content_size(scroll).h;

	scroll_layer_set_content_offset(scroll, GPoint(0, scrollTo), false);
	scroll_layer_set_content_offset(scroll, GPoint(0, scrollTo), true);
}

static void scroll_notification_by(int16_t amount)
{
	GSize size = scroll_layer_get_content_size(scroll);
	GPoint point = scroll_layer_get_content_offset(scroll);
	point.y += amount;
	if (point.y < -size.h)
		point.y = -size.h;

	scroll_layer_set_content_offset(scroll, point, true);
}

static void set_busy_indicator(bool value)
{
	busy = value;
	layer_mark_dirty(statusbar);
}

static void remove_notification(uint8_t id, bool closeAutomatically)
{
	if (numOfNotifications <= 1 && closeAutomatically)
	{
		tertiary_text_window_close();
		window_stack_pop(true);
		return;
	}

	if (numOfNotifications > 0)
		numOfNotifications--;

	uint8_t pos = notificationPositions[id];

	notificationDataUsed[pos] = false;

	for (int i = id; i < numOfNotifications; i++)
	{
		notificationPositions[i] = notificationPositions[i + 1];
	}

	if (pickedNotification >= numOfNotifications && pickedNotification > 0)
	{
		bool refresh = (pickedNotification == id);

		pickedNotification--;

		if (refresh)
		{
			actions_menu_hide();
			tertiary_text_window_close();
			refresh_notification();
			scroll_to_notification_start();
		}

	}
}

static Notification* add_notification(void)
{
	if (numOfNotifications >= NOTIFICATION_SLOTS)
		remove_notification(0, false);

	uint8_t position = 0;
	for (int i = 0; i < NOTIFICATION_SLOTS; i++)
	{
		if (!notificationDataUsed[i])
		{
			position = i;
			break;
		}
	}

	notificationDataUsed[position] = true;
	notificationPositions[numOfNotifications] = position;
	numOfNotifications++;

	layer_mark_dirty(circlesLayer);

	return &notificationData[position];
}

static Notification* find_notification(int32_t id)
{
	for (int i = 0; i < NOTIFICATION_SLOTS; i++)
	{
		if (notificationDataUsed[i] && notificationData[i].id == id)
			return &notificationData[i];
	}

	return NULL;
}

static void send_message_action_menu_result(int action)
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

	set_busy_indicator(true);
	actions_menu_hide();
}

void send_message_reply_writing_result(char* text)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 4);
	dict_write_uint8(iterator, 1, 3);
	dict_write_cstring(iterator, 2, text);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

	set_busy_indicator(true);
}

static void send_message_next_list_notification(int8_t change)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 2);
	dict_write_uint8(iterator, 1, 2);
	dict_write_int8(iterator, 2, change);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

	set_busy_indicator(true);
}

static void notification_sendSelectAction(int32_t notificationId, bool hold)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 4);
	dict_write_uint8(iterator, 1, 0);

	dict_write_int32(iterator, 2, notificationId);
	if (hold)
		dict_write_uint8(iterator, 3, 1);

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

	set_busy_indicator(true);
}

static void dismiss_notification_from_phone(int32_t notificationId)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 4);
	dict_write_uint8(iterator, 1, 1);

	dict_write_int32(iterator, 2, notificationId);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

	set_busy_indicator(true);
}

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

	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return;

	if (actions_menu_is_displayed())
	{
		send_message_action_menu_result(actions_menu_get_selected_index());
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

		notification_sendSelectAction(curNotification->id, false);
	}
}

static void button_center_hold(ClickRecognizerRef recognizer, void* context)
{
	if (actions_menu_is_displayed())
		return;

	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return;

	if (busy)
		return;

	if (curNotification->showMenuOnSelectHold)
	{
		actions_menu_set_number_of_items(curNotification->numOfActionsInDefaultMenu);
		actions_menu_reset_text();
		actions_menu_show();
	}

	notification_sendSelectAction(curNotification->id, true);
}

static void button_up_raw_pressed(ClickRecognizerRef recognizer, void* context)
{
	if (actions_menu_is_displayed())
	{
		actions_menu_move_up();
	}
	else
	{
		scroll_layer_scroll_up_click_handler(recognizer, scroll);
	}

	appIdle = false;
	upPressed = true;
	skippedUpPresses = 0;

}

static void button_down_raw_pressed(ClickRecognizerRef recognizer, void* context)
{
	app_log(0, "notify", 0, "%d", heap_bytes_free());

	if (actions_menu_is_displayed())
	{
		actions_menu_move_down();
	}
	else
	{
		scroll_layer_scroll_down_click_handler(recognizer, scroll);
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
			scroll_notification_by(50);
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
			scroll_notification_by(-50);
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
		Notification data = notificationData[notificationPositions[pickedNotification]];
		if (data.inList && !busy)
		{
			send_message_next_list_notification(-1);
			return;
		}
	}

	if (numOfNotifications == 1)
	{
		scroll_layer_scroll_up_click_handler(recognizer, scroll);
		return;
	}

	if (pickedNotification == 0)
		pickedNotification = numOfNotifications - 1;
	else
		pickedNotification--;

	refresh_notification();
	scroll_to_notification_start();
}

static void button_down_double(ClickRecognizerRef recognizer, void* context)
{
	if (actions_menu_is_displayed())
		return;

	if (pickedNotification == numOfNotifications - 1)
	{
		Notification data = notificationData[notificationPositions[pickedNotification]];
		if (data.inList && !busy)
		{
			send_message_next_list_notification(1);
			return;
		}
	}

	if (numOfNotifications == 1)
	{
		scroll_layer_scroll_down_click_handler(recognizer, scroll);
		return;
	}

	if (pickedNotification == numOfNotifications - 1)
		pickedNotification = 0;
	else
		pickedNotification++;

	refresh_notification();
	scroll_to_notification_start();
}

static void registerButtons(void* context) {
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

static void vibration_stopped(void* data)
{
	vibrating = false;
}

static void received_message_new_notification(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 2)->value->int32;

	uint8_t* configBytes = dict_find(received, 3)->value->data;

	uint8_t flags = configBytes[0];
	bool inList = (flags & 0x02) != 0;
	bool autoSwitch = (flags & 0x04) != 0;

	uint8_t numOfVibrationBytes = configBytes[4];

	uint16_t newPeriodicVibration = configBytes[1] << 8 | configBytes[2];
	if (newPeriodicVibration < periodicVibrationPeriod || periodicVibrationPeriod == 0)
		periodicVibrationPeriod = newPeriodicVibration;

	Notification* notification = find_notification(id);
	if (notification == NULL)
	{
		notification = add_notification();

		if (!inList)
		{
			if (canVibrate())
			{
				bool vibrate = false;

				uint16_t totalLength = 0;
				uint32_t segments[20];
				for (int i = 0; i < numOfVibrationBytes; i+= 2)
				{
					segments[i / 2] = configBytes[5 +i] | (configBytes[6 +i] << 8);
					totalLength += segments[i / 2];
					if (i % 4 == 0 && segments[i / 2] > 0)
						vibrate = true;
				}
				if (vibrate)
				{
					VibePattern pat = {
					.durations = segments,
					.num_segments = numOfVibrationBytes / 2,
					};
					vibes_enqueue_custom_pattern(pat);

					vibrating = true;
					app_timer_register(totalLength, vibration_stopped, NULL);
				}
			}

			if (config_lightScreen)
				light_enable_interaction();

			appIdle = true;
			elapsedTime = 0;
		}
	}

	notification->id = id;
	notification->inList = inList;
	notification->scrollToEnd = (flags & 0x08) != 0;
	notification->showMenuOnSelectPress = (flags & 0x10) != 0;
	notification->showMenuOnSelectHold = (flags & 0x20) != 0;
	notification->numOfActionsInDefaultMenu = configBytes[3];
	strcpy(notification->title, dict_find(received, 4)->value->cstring);
	strcpy(notification->subTitle, dict_find(received, 5)->value->cstring);
	notification->text[0] = 0;

	if (notification->inList)
	{
		for (int i = 0; i < numOfNotifications; i++)
		{
			Notification* entry = &notificationData[notificationPositions[i]];
			if (entry->id == notification->id)
				continue;

			if (entry->inList)
			{
				remove_notification(i, false);
				i--;
			}
		}
	}

	if (autoSwitch && !actions_menu_is_displayed())
		pickedNotification = numOfNotifications - 1;

	if (pickedNotification == numOfNotifications - 1)
	{
		refresh_notification();
		scroll_to_notification_start();
	}

	set_busy_indicator(false);

}

static void received_message_dismiss(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 2)->value->int32;
	for (int i = 0; i < numOfNotifications; i++)
	{
		Notification entry = notificationData[notificationPositions[i]];
		if (entry.id != id)
			continue;

		remove_notification(i, true);

		set_busy_indicator(false);

		break;
	}
}

static void received_message_more_text(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 2)->value->int32;

	Notification* notification = find_notification(id);
	if (notification == NULL)
		return;

	uint16_t length = strlen(notification->text);
	strcpy(notification->text + length, dict_find(received, 3)->value->cstring);

	if (pickedNotification == numOfNotifications - 1)
		refresh_notification();
}

static void received_message_notification_list_items(DictionaryIterator *received)
{
	if (actions_menu_got_items(received))
		set_busy_indicator(false);
}



void notification_window_received_data(uint8_t module, uint8_t id, DictionaryIterator *received) {
	if (module == 0 && id == 1)
	{
		set_busy_indicator(false);
	}
	else if (module == 1)
	{
		if (id == 0)
			received_message_new_notification(received);
		else if (id == 1)
			received_message_more_text(received);
	}
	else if (module == 3 && id == 0)
	{
		received_message_dismiss(received);
	}
	else if (module == 4 && id == 0)
	{
		received_message_notification_list_items(received);
	}
}

void notification_window_data_sent(void)
{
	if (pickedAction > -1)
		send_message_action_menu_result(pickedAction);
}

static void statusbarback_paint(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, 144, 16), 0, GCornerNone);

	if (busy)
		graphics_draw_bitmap_in_rect(ctx, busyIndicator, GRect(80, 3, 9, 10));
}


static void circles_paint(Layer *layer, GContext *ctx)
{
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	int x = 9;
	for (int i = 0; i < numOfNotifications; i++)
	{
		if (pickedNotification == i)
			graphics_fill_circle(ctx, GPoint(x, 8), 4);
		else
			graphics_draw_circle(ctx, GPoint(x, 8), 4);

		x += 11;
	}
}

static void updateStatusClock(void)
{
	time_t now = time(NULL);
	struct tm* lTime = localtime(&now);

	char* formatString;
	if (clock_is_24h_style())
		formatString = "%H:%M";
	else
		formatString = "%I:%M %p";

	strftime(clockText, 9, formatString, lTime);
	//text_layer_set_text(statusClock, "99:99 PM");
	text_layer_set_text(statusClock, clockText);
}

static void accelerometer_shake(AccelAxisType axis, int32_t direction)
{
	if (vibrating) //Vibration seems to generate a lot of false positives
		return;

	if (config_shakeAction == 1)
		appIdle = false;
	else if (config_shakeAction == 2)
	{
		Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
		if (curNotification == NULL)
			return;

		dismiss_notification_from_phone(curNotification->id);
	}
}


static void second_tick(void)
{
	elapsedTime++;

	if (appIdle && config_timeout > 0 && config_timeout < elapsedTime && main_noMenu)
	{
		window_stack_pop(true);
		return;
	}

	if (periodicVibrationPeriod > 0 &&
		appIdle &&
		elapsedTime > 0 && elapsedTime % periodicVibrationPeriod == 0 &&
		!notificationData[notificationPositions[pickedNotification]].inList &&
		canVibrate() &&
		(config_periodicTimeout == 0 || elapsedTime < config_periodicTimeout))
	{
		vibrating = true;
		app_timer_register(500, vibration_stopped, NULL);
		vibes_short_pulse();
	}

	updateStatusClock();
}


static TextLayer* init_text_layer(int fontId)
{
	TextLayer* layer = text_layer_create(GRect(0, 0, 0, 0)); //Size is set by notification_refresh() so it is not important here
	text_layer_set_overflow_mode(layer, GTextOverflowModeWordWrap);
	scroll_layer_add_child(scroll, (Layer*) layer);

	text_layer_set_font(layer, fonts_get_system_font(config_getFontResource(fontId)));

	return layer;
}

static void window_appears(Window *window)
{
	setCurWindow(1);

	updateStatusClock();

	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) second_tick);
}

static void window_load(Window *window)
{
	app_log(0, "notify", 100, "%d", heap_bytes_free());

	busyIndicator = gbitmap_create_with_resource(RESOURCE_ID_INDICATOR_BUSY);

	Layer* topLayer = window_get_root_layer(notifyWindow);

	statusbar = layer_create(GRect(0, 0, 144, 16));
	layer_set_update_proc(statusbar, statusbarback_paint);
	layer_add_child(topLayer, statusbar);

	circlesLayer = layer_create(GRect(0, 0, 144 - 65, 16));
	layer_set_update_proc(circlesLayer, circles_paint);
	layer_add_child(statusbar, circlesLayer);

	statusClock = text_layer_create(GRect(144 - 53, 0, 50, 16));
	text_layer_set_background_color(statusClock, GColorBlack);
	text_layer_set_text_color(statusClock, GColorWhite);
	text_layer_set_font(statusClock, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(statusClock, GTextAlignmentRight);
	layer_add_child(statusbar, (Layer*) statusClock);

	scroll = scroll_layer_create(GRect(0, 16, 144, 168 - 16));
	layer_add_child(topLayer, (Layer*) scroll);

	title = init_text_layer(config_titleFont);
	subTitle = init_text_layer(config_subtitleFont);
	text = init_text_layer(config_bodyFont);

	actions_menu_init();
	actions_menu_attach(topLayer);

	if (config_invertColors)
	{
		inverterLayer = inverter_layer_create(layer_get_frame(topLayer));
		layer_add_child(topLayer, (Layer*) inverterLayer);
	}

	if (config_shakeAction > 0)
		accel_tap_service_subscribe(accelerometer_shake);

	refresh_notification();
	app_log(0, "notify", 200, "%d", heap_bytes_free());

}

static void window_unload(Window *window)
{
	layer_destroy(statusbar);
	layer_destroy(circlesLayer);
	text_layer_destroy(title);
	text_layer_destroy(subTitle);
	text_layer_destroy(text);
	text_layer_destroy(statusClock);
	scroll_layer_destroy(scroll);
	gbitmap_destroy(busyIndicator);
	actions_menu_deinit();

	if (inverterLayer != NULL)
		inverter_layer_destroy(inverterLayer);

	if (config_shakeAction > 0)
		accel_tap_service_unsubscribe();

	window_destroy(window);
	setCurWindow(-1);

	tick_timer_service_unsubscribe();

	if (main_noMenu && config_dontClose)
	{
		closeApp();
	}

	if (main_noMenu)
		closingMode = true;
}

void notification_window_init(void)
{
	notifyWindow = window_create();

	window_set_window_handlers(notifyWindow, (WindowHandlers) {
		.appear = (WindowHandler)window_appears,
				.load = (WindowHandler) window_load,
				.unload = (WindowHandler) window_unload
	});


	numOfNotifications = 0;
	for (int i = 0; i < NOTIFICATION_SLOTS; i++)
	{
		notificationDataUsed[i] = false;
	}

	window_set_click_config_provider(notifyWindow, (ClickConfigProvider) registerButtons);

	window_set_fullscreen(notifyWindow, true);
	window_stack_push(notifyWindow, true);

	if (!config_dontClose)
		main_menu_close();
}
