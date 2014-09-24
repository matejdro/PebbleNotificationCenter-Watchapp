#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"

typedef struct
{
	int32_t id;
	bool dismissable;
	bool inList;
	bool showActionMenu;
	uint8_t numOfChunks;
	uint8_t numOfActions;
	char title[31];
	char subTitle[31];
	char actionNames[5][20];
	char text[900];

} Notification;


uint32_t elapsedTime = 0;
bool appIdle = true;
bool vibrating = false;

bool closeOnReceive = false;
bool exitOnClose = false;
bool closeSent = false;

uint8_t periodicVibrationPeriod = 0;

Window* notifyWindow;

static InverterLayer* inverterLayer;

Layer* statusbar;
TextLayer* statusClock;
char clockText[9];

Layer* circlesLayer;

bool busy;
GBitmap* busyIndicator;

uint8_t numOfNotifications = 0;
uint8_t pickedNotification = 0;

Notification notificationData[5];
uint8_t notificationPositions[5];
bool notificationDataUsed[5];

ScrollLayer* scroll;

bool upPressed = false;
bool downPressed = false;

TextLayer* title;
TextLayer* subTitle;
TextLayer* text;

Layer* menuBackground;
MenuLayer* actionsMenu;
bool actionsMenuDisplayed = false;

bool stopBusyAfterSend = false;

void registerButtons(void* context);

void refresh_notification()
{
	char* titleText = "";
	char* subtitleText = "";
	char* bodyText = "";

	if (numOfNotifications < 1)
	{
		titleText = "No notifications";
		subtitleText = "";
		bodyText = "";
	}
	else
	{
		Notification* notification = &notificationData[notificationPositions[pickedNotification]];
		titleText = notification->title;
		subtitleText = notification->subTitle;
		bodyText = notification->text;
	}

	//	GSize titleSize = graphics_text_layout_get_max_used_size(app_get_current_graphics_context(), titleText, fonts_get_system_font(titleFont), GRect(2, 0, 144 - 4, 30000), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	//	GSize subtitleSize = graphics_text_layout_get_max_used_size(app_get_current_graphics_context(), subtitleText, fonts_get_system_font(subtitleFont), GRect(2, 0, 144 - 4, 30000), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
	//	GSize textSize = graphics_text_layout_get_max_used_size(app_get_current_graphics_context(), bodyText, fonts_get_system_font(textFont), GRect(2, 0, 144 - 4, 30000), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

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

	layer_set_frame((Layer*) subTitle, GRect(2, titleSize.h + 1, 144 - 4, subtitleSize.h));
	layer_set_frame((Layer*) text, GRect(2, titleSize.h + 1 + subtitleSize.h + 1, 144 - 4, textSize.h));

	scroll_layer_set_content_size(scroll, GSize(144 - 4, titleSize.h + 1 + subtitleSize.h + 1 + textSize.h + 5));
	scroll_layer_set_content_offset(scroll, GPoint(0, 0), false);
	scroll_layer_set_content_offset(scroll, GPoint(0, 0), true);


	layer_mark_dirty(circlesLayer);
}

void set_busy_indicator(bool value)
{
	busy = value;
	layer_mark_dirty(statusbar);
}

void notification_remove_notification(uint8_t id, bool closeAutomatically)
{
	if (numOfNotifications <= 1 && closeAutomatically)
	{
		closingMode = true;
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
		pickedNotification--;

	refresh_notification();
}

Notification* notification_add_notification()
{
	if (numOfNotifications >= 5)
		notification_remove_notification(0, false);

	uint8_t position = 0;
	for (int i = 0; i < 8; i++)
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

Notification* notification_find_notification(int32_t id)
{
	for (int i = 0; i < 8; i++)
	{
		if (notificationDataUsed[i] && notificationData[i].id == id)
			return &notificationData[i];
	}

	return NULL;
}

void notification_action(Notification* notification, int action)
{
	if (!notification->dismissable)
		action++;

	bool dismiss = action < 2; //It is not guaranteed that other actions will actually dismiss notification

	if (action != 1) //1 = just hide from Pebble
	{
		DictionaryIterator *iterator;
		AppMessageResult result = app_message_outbox_begin(&iterator);
		if (result != APP_MSG_OK)
			return;

		dict_write_uint8(iterator, 0, 3);
		dict_write_int32(iterator, 1, notification->id);
		dict_write_uint8(iterator, 3, action);

		if (dismiss && numOfNotifications <= 1 && exitOnClose)
		{
			refresh_notification();

			dict_write_uint8(iterator, 2, 0);
			closeSent = true;
		}
		app_message_outbox_send();

		set_busy_indicator(true);
		stopBusyAfterSend = true;
	}

	if (dismiss)
		notification_remove_notification(pickedNotification, true);
}

static void menu_show()
{
	menu_layer_reload_data(actionsMenu);
	MenuIndex firstItem;
	firstItem.row = 0;
	firstItem.section = 0;
	menu_layer_set_selected_index(actionsMenu, firstItem, MenuRowAlignTop, false);
	actionsMenuDisplayed = true;
	layer_set_hidden((Layer*) menuBackground, false);
}

static void menu_hide()
{
	actionsMenuDisplayed = false;
	layer_set_hidden((Layer*) menuBackground, true);
}

void notification_back_single(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
		menu_hide();
	else
		window_stack_pop(true);
}

void notification_center_single(ClickRecognizerRef recognizer, void* context)
{
	appIdle = false;

	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return;

	if (actionsMenuDisplayed)
	{
		notification_action(curNotification, menu_layer_get_selected_index(actionsMenu).row);
		menu_hide();
	}
	else
	{
		menu_show();
	}

	//notification_dismiss_current(curNotification, true);
}

void notification_up_rawPressed(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
	{
		menu_layer_set_selected_next(actionsMenu, true, MenuRowAlignCenter, true);
		return;
	}

	appIdle = false;
	upPressed = true;
	scroll_layer_scroll_up_click_handler(recognizer, scroll);

}
void notification_down_rawPressed(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
	{
		menu_layer_set_selected_next(actionsMenu, false, MenuRowAlignCenter, true);
		return;
	}

	appIdle = false;
	downPressed = true;
	scroll_layer_scroll_down_click_handler(recognizer, scroll);
}
void notification_up_rawReleased(ClickRecognizerRef recognizer, void* context)
{
	upPressed = false;
}
void notification_down_rawReleased(ClickRecognizerRef recognizer, void* context)
{
	downPressed = false;
}
void notification_up_click_proxy(ClickRecognizerRef recognizer, void* context)
{
	if (upPressed)
	{
		if (actionsMenuDisplayed)
		{
			menu_layer_set_selected_next(actionsMenu, true, MenuRowAlignCenter, true);
			return;
		}

		scroll_layer_scroll_up_click_handler(recognizer, scroll);
	}
}
void notification_down_click_proxy(ClickRecognizerRef recognizer, void* context)
{
	if (downPressed)
	{
		if (actionsMenuDisplayed)
		{
			menu_layer_set_selected_next(actionsMenu, false, MenuRowAlignCenter, true);
			return;
		}

		scroll_layer_scroll_down_click_handler(recognizer, scroll);
	}
}

void notification_up_double(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
		return;

	if (pickedNotification == 0)
	{
		Notification data = notificationData[notificationPositions[pickedNotification]];
		if (data.inList)
		{
			DictionaryIterator *iterator;
			app_message_outbox_begin(&iterator);
			dict_write_uint8(iterator, 0, 8);
			dict_write_int8(iterator, 1, -1);
			app_message_outbox_send();

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
}

void notification_down_double(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
		return;

	if (pickedNotification == numOfNotifications - 1)
	{
		Notification data = notificationData[notificationPositions[pickedNotification]];
		if (data.inList)
		{
			DictionaryIterator *iterator;
			app_message_outbox_begin(&iterator);
			dict_write_uint8(iterator, 0, 8);
			dict_write_int8(iterator, 1, 1);
			app_message_outbox_send();

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
}

void registerButtons(void* context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) notification_center_single);
	window_single_click_subscribe(BUTTON_ID_BACK, (ClickHandler) notification_back_single);

	window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 150, false, (ClickHandler) notification_up_double);
	window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 150, false, (ClickHandler) notification_down_double);

	window_raw_click_subscribe(BUTTON_ID_UP, (ClickHandler) notification_up_rawPressed, (ClickHandler) notification_up_rawReleased, NULL);
	window_raw_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) notification_down_rawPressed, (ClickHandler) notification_down_rawReleased, NULL);

	window_single_repeating_click_subscribe(BUTTON_ID_UP, 200, (ClickHandler) notification_up_click_proxy);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 200, (ClickHandler) notification_down_click_proxy);
}

static void menu_paint_background(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, 144 - 18, 168 - 34), 0, GCornerNone);
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(1, 1, 144 - 20, 168 - 36), 0, GCornerNone);

}

static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data) {
	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return 0;

	int initialSize = curNotification->dismissable ? 3 : 2;

	return initialSize + curNotification->numOfActions;
}


static int16_t menu_get_row_height_callback(MenuLayer *me,  MenuIndex *cell_index, void *data) {
	return 30;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	graphics_context_set_text_color(ctx, GColorBlack);

	char* text;
	int idAdd = 0;

	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return;

	if (curNotification->dismissable)
	{
		idAdd = 1;
		if (cell_index->row == 0)
		{
			text = "Dismiss on phone";
		}
	}

	if (cell_index->row == 0 + idAdd)
	{
		text = "Hide from Pebble";
	}
	else if (cell_index->row == 1 + idAdd)
	{
		text = "Open on phone";
	}
	else if (cell_index->row > 1 + idAdd)
	{
		text = curNotification->actionNames[cell_index->row - (idAdd + 2)];
	}

	graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(5, 0, 144 - 20 - 10, 27), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);


}


static void menu_select_callback(MenuLayer *me, MenuIndex *cell_index, void *data) {
}


void vibration_stopped(void* data)
{
	vibrating = false;
}

void notification_sendMoreText(int32_t id, uint8_t offset)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 1);
	dict_write_int32(iterator, 1, id);
	dict_write_uint8(iterator, 2, offset);
	app_message_outbox_send();
}

void notification_sendActionNames(int32_t id)
{
	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 12);
	dict_write_int32(iterator, 1, id);
	app_message_outbox_send();
}


void notification_sendNextNotification()
{
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 2);
	app_message_outbox_send();

	stopBusyAfterSend = true;
}

void notification_newNotification(DictionaryIterator *received)
{
	set_busy_indicator(true);

	int32_t id = dict_find(received, 1)->value->int32;

	uint8_t* configBytes = dict_find(received, 2)->value->data;

	uint8_t flags = configBytes[1];
	bool inList = (flags & 0x02) != 0;
	bool autoSwitch = (flags & 0x04) != 0;

	uint8_t numOfVibrationBytes = configBytes[3];

	Notification* notification = notification_find_notification(id);
	if (notification == NULL)
	{
		notification = notification_add_notification();

		if (!inList)
		{
			periodicVibrationPeriod = configBytes[2];

			uint32_t segments[20];
			for (int i = 0; i < numOfVibrationBytes; i+= 2)
			{
				segments[i / 2] = configBytes[4 +i] | (configBytes[5 +i] << 8);
			}
			VibePattern pat = {
			.durations = segments,
			.num_segments = numOfVibrationBytes / 2,
			};
			vibes_enqueue_custom_pattern(pat);

			if (config_lightScreen)
				light_enable_interaction();

			appIdle = true;
			elapsedTime = 0;
		}
	}

	notification->id = id;
	notification->inList = inList;
	notification->dismissable = (flags & 0x01) != 0;
	notification->numOfChunks = dict_find(received, 4)->value->uint8;
	notification->numOfActions = configBytes[4 + numOfVibrationBytes];
	strcpy(notification->title, dict_find(received, 5)->value->cstring);
	strcpy(notification->subTitle, dict_find(received, 6)->value->cstring);
	notification->text[0] = 0;

	if (notification->inList)
	{
		for (int i = 0; i < numOfNotifications; i++)
		{

			Notification entry = notificationData[notificationPositions[i]];
			if (entry.id == notification->id)
				continue;

			if (entry.inList)
			{
				notification_remove_notification(i, false);
				i--;
			}
		}
	}

	if (notification->numOfActions > 0)
	{
		notification_sendActionNames(notification->id);

		//Specify placeholder action names
		for (int i = 0; i < notification->numOfActions; i++)
		{
			snprintf(notification->actionNames[i], 20, "Action %d", i + 1);
		}
	}
	else
	{
		if (notification->numOfChunks == 0)
		{
			notification_sendNextNotification();
		}
		else
		{
			notification_sendMoreText(notification->id, 0);
		}
	}


	if (numOfNotifications == 1)
		refresh_notification();
	else if (autoSwitch)
	{
		pickedNotification = numOfNotifications - 1;
		refresh_notification();
	}
}

void notification_gotDismiss(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 1)->value->int32;
	bool close = dict_find(received, 2) == NULL;

	for (int i = 0; i < numOfNotifications; i++)
	{

		Notification entry = notificationData[notificationPositions[i]];
		if (entry.id != id)
			continue;

		notification_remove_notification(i, false);

		break;
	}

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 9);
	if (numOfNotifications < 1 && exitOnClose && close)
	{
		refresh_notification();

		dict_write_uint8(iterator, 2, 0);
	}
	app_message_outbox_send();
	set_busy_indicator(true);
	stopBusyAfterSend = true;
}

void notification_gotMoreText(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 1)->value->int32;

	Notification* notification = notification_find_notification(id);
	if (notification == NULL)
	{
		notification_sendNextNotification();
		return;
	}

	uint8_t chunk = dict_find(received, 2)->value->uint8;

	uint16_t length = strlen(notification->text);
	strcpy(notification->text + length, dict_find(received, 3)->value->cstring);

	if (++chunk >= notification->numOfChunks)
	{
		notification_sendNextNotification();
	}
	else
	{
		notification_sendMoreText(id, chunk);
	}

	if (pickedNotification == numOfNotifications - 1)
		refresh_notification();
}

void notification_gotActionNames(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 1)->value->int32;

	Notification* notification = notification_find_notification(id);
	if (notification == NULL)
	{
		notification_sendNextNotification();
		return;
	}

	uint8_t* text = dict_find(received, 3)->value->data;

	for (int i = 0; i < notification->numOfActions; i++)
	{
		memcpy(notification->actionNames[i], &text[i*19],  19);
	}

	if (notification->numOfChunks == 0)
	{
		notification_sendNextNotification();
	}
	else
	{
		notification_sendMoreText(id, 0);
	}

	if (pickedNotification == numOfNotifications - 1)
		menu_layer_reload_data(actionsMenu);
}


void notification_received_data(uint8_t id, DictionaryIterator *received) {
	switch (id)
	{
	case 0:
		notification_newNotification(received);
		break;
	case 1:
		notification_gotMoreText(received);
		break;
	case 4:
		notification_gotDismiss(received);
		break;
	case 5:
		notification_gotActionNames(received);
		break;
	}


}

void notification_data_sent(DictionaryIterator *received, void *context)
{
	if (stopBusyAfterSend)
	{
		stopBusyAfterSend = false;
		set_busy_indicator(false);
	}

	if (closeOnReceive)
	{
		closingMode = true;
		window_stack_pop(true);
	}
}

void statusbarback_paint(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, 144, 16), 0, GCornerNone);

	if (busy)
		graphics_draw_bitmap_in_rect(ctx, busyIndicator, GRect(80, 3, 9, 10));
}


void circles_paint(Layer *layer, GContext *ctx)
{
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);

	int x = 7;
	for (int i = 0; i < numOfNotifications; i++)
	{
		if (pickedNotification == i)
			graphics_fill_circle(ctx, GPoint(x, 8), 3);
		else
			graphics_draw_circle(ctx, GPoint(x, 8), 3);

		x += 9;
	}
}

void updateStatusClock()
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

void accelerometer_shake(AccelAxisType axis, int32_t direction)
{
	if (vibrating) //Vibration seems to generate a lot of false positives
		return;

	if (config_shakeAction == 1)
		appIdle = false;
	else if (config_shakeAction == 2)
		notification_center_single(NULL, NULL);
}


void notification_second_tick()
{
	elapsedTime++;

	if (appIdle && config_timeout > 0 && config_timeout < elapsedTime && exitOnClose)
	{
		window_stack_pop(true);
		return;
	}

	if (periodicVibrationPeriod > 0 && appIdle && elapsedTime > 0 && elapsedTime % periodicVibrationPeriod == 0 && !notificationData[notificationPositions[pickedNotification]].inList && (!config_dontVibrateWhenCharging || !battery_state_service_peek().is_charging))
	{
		vibrating = true;
		app_timer_register(500, vibration_stopped, NULL);
		vibes_short_pulse();
	}

	updateStatusClock();
}

void notification_appears(Window *window)
{
	setCurWindow(1);

	updateStatusClock();

	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) notification_second_tick);
}

void notification_disappears(Window *window)
{
	tick_timer_service_unsubscribe();
}

void notification_load(Window *window)
{
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

	title = text_layer_create(GRect(2, 0, 144 - 4, 18));
	text_layer_set_font(title, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_overflow_mode(title, GTextOverflowModeWordWrap);
	text_layer_set_background_color(title, GColorWhite);
	scroll_layer_add_child(scroll, (Layer*) title);

	subTitle = text_layer_create(GRect(2, 18, 144 - 4, 16));
	text_layer_set_font(subTitle, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_overflow_mode(subTitle, GTextOverflowModeWordWrap);
	text_layer_set_background_color(title, GColorWhite);
	scroll_layer_add_child(scroll, (Layer*) subTitle);

	text = text_layer_create(GRect(2, 18 + 16, 144 - 4, 16));
	text_layer_set_font(text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_overflow_mode(text, GTextOverflowModeWordWrap);
	text_layer_set_background_color(title, GColorWhite);
	scroll_layer_add_child(scroll, (Layer*) text);

	text_layer_set_font(title, fonts_get_system_font(config_getFontResource(config_titleFont)));
	text_layer_set_font(subTitle, fonts_get_system_font(config_getFontResource(config_subtitleFont)));
	text_layer_set_font(text, fonts_get_system_font(config_getFontResource(config_bodyFont)));

	menuBackground = layer_create(GRect(9, 25, 144 - 18, 168 - 34));
	layer_set_update_proc(menuBackground, menu_paint_background);
	layer_set_hidden((Layer*) menuBackground, true);
	layer_add_child(topLayer, menuBackground);

	actionsMenu = menu_layer_create(GRect(1, 1, 144 - 20, 168 - 36));
	layer_add_child(menuBackground, (Layer*) actionsMenu);
	menu_layer_set_callbacks(actionsMenu, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_cell_height = menu_get_row_height_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,

	});

	if (config_invertColors)
	{
		inverterLayer = inverter_layer_create(layer_get_frame(topLayer));
		layer_add_child(topLayer, (Layer*) inverterLayer);
	}

	if (config_shakeAction > 0)
		accel_tap_service_subscribe(accelerometer_shake);

}

void notification_unload(Window *window)
{
	layer_destroy(statusbar);
	layer_destroy(circlesLayer);
	text_layer_destroy(title);
	text_layer_destroy(subTitle);
	text_layer_destroy(text);
	scroll_layer_destroy(scroll);
	gbitmap_destroy(busyIndicator);
	menu_layer_destroy(actionsMenu);

	if (inverterLayer != NULL)
		inverter_layer_destroy(inverterLayer);

	if (config_shakeAction > 0)
		accel_tap_service_unsubscribe();

	window_destroy(window);

	if (exitOnClose && !closeSent)
		closeApp();
}

void notification_window_init(bool liveNotification)
{
	exitOnClose = liveNotification;

	notifyWindow = window_create();

	window_set_window_handlers(notifyWindow, (WindowHandlers) {
		.appear = (WindowHandler)notification_appears,
				.disappear = (WindowHandler) notification_disappears,
				.load = (WindowHandler) notification_load,
				.unload = (WindowHandler) notification_unload
	});


	numOfNotifications = 0;
	for (int i = 0; i < 5; i++)
	{
		notificationDataUsed[i] = false;
	}

	window_set_click_config_provider(notifyWindow, (ClickConfigProvider) registerButtons);

	window_set_fullscreen(notifyWindow, true);
	window_stack_push(notifyWindow, true);

}
