#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "MainMenu.h"

typedef struct
{
	int32_t id;
	bool dismissable;
	bool inList;
	uint8_t numOfChunks;
	char title[31];
	char subTitle[31];
	char text[1001];

} Notification;

uint8_t textSize = 0;
uint16_t setTimeout = 0;
bool vibratePeriodically = true;

uint32_t elapsedTime = 0;
bool appIdle = true;
bool closeCommandSent = false;

bool closeOnReceive = false;
bool exitOnClose = false;
bool enableCloseWindow = false;

Window* notifyWindow;

Layer* statusbar;
TextLayer* statusClock;
char clockText[9];

Layer* circlesLayer;

bool busy;
GBitmap* busyIndicator;

uint8_t numOfNotifications = 0;
uint8_t pickedNotification = 0;

Notification notificationData[8];
uint8_t notificationPositions[8];
bool notificationDataUsed[8];

ScrollLayer* scroll;

bool upPressed = false;
bool downPressed = false;

TextLayer* title;
TextLayer* subTitle;
TextLayer* text;

bool stopBusyAfterSend = false;

char *itoa(int32_t num)
{
	if (num == 0)
		return "0";
	static char buff[20] = {};
	int32_t i = 0;
	int32_t temp_num = num;
	int32_t length = 0;
	char *string = buff;
	if(num >= 0) {
		// count how many characters in the number
		while(temp_num) {
			temp_num /= 10;
			length++;
		}
		// assign the number to the buffer starting at the end of the
		// number and going to the begining since we are doing the
		// integer to character conversion on the last number in the
		// sequence
		for(i = 0; i < length; i++) {
			buff[(length-1)-i] = '0' + (num % 10);
			num /= 10;
		}
		buff[i] = '\0'; // can't forget the null byte to properly end our string
	}
	else
		return "ER";
	return string;
}

void refresh_notification()
{
	char* titleFont;
	char* subtitleFont;
	char* textFont;
	switch (textSize)
	{
	case 1:
		titleFont = FONT_KEY_GOTHIC_24_BOLD;
		subtitleFont = FONT_KEY_GOTHIC_18_BOLD;
		textFont = FONT_KEY_GOTHIC_18;
		break;
	case 2:
		titleFont = FONT_KEY_GOTHIC_28_BOLD;
		subtitleFont = FONT_KEY_GOTHIC_24_BOLD;
		textFont = FONT_KEY_GOTHIC_24;
		break;
	default:
		titleFont = FONT_KEY_GOTHIC_18_BOLD;
		subtitleFont = FONT_KEY_GOTHIC_14_BOLD;
		textFont = FONT_KEY_GOTHIC_14;
		break;
	}

	char* titleText = "";
	char* subtitleText = "";
	char* bodyText = "";

	if (closeCommandSent || numOfNotifications < 1)
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

	text_layer_set_font(title, fonts_get_system_font(titleFont));
	text_layer_set_font(subTitle, fonts_get_system_font(subtitleFont));
	text_layer_set_font(text, fonts_get_system_font(textFont));

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

	layer_mark_dirty(circlesLayer);
}

void closeApp()
{
	closeCommandSent = true;
	refresh_notification();
	scroll_layer_set_content_offset(scroll, GPoint(0,0), false);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 7);
	app_message_outbox_send();
}

void set_busy_indicator(bool value)
{
	busy = value;
	layer_mark_dirty(statusbar);
}

void notification_remove_notification(uint8_t id, bool waitForSending, bool close)
{
	if (numOfNotifications <= 1 && close)
	{
		if (!waitForSending)
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
	if (numOfNotifications >= 8)
		notification_remove_notification(0, false, true);

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

void notification_center_single(ClickRecognizerRef recognizer, void* context)
{
	appIdle = false;

	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return;

	if (curNotification->dismissable)
	{
		DictionaryIterator *iterator;
		AppMessageResult result = app_message_outbox_begin(&iterator);
		if (result != APP_MSG_OK)
			return;

		dict_write_uint8(iterator, 0, 3);
		dict_write_int32(iterator, 1, curNotification->id);
		if (numOfNotifications <= 1 && exitOnClose)
		{
			closeCommandSent = true;
			refresh_notification();
			scroll_layer_set_content_offset(scroll, GPoint(0,0), false);

			dict_write_uint8(iterator, 2, 0);
		}
		app_message_outbox_send();

		set_busy_indicator(true);
		stopBusyAfterSend = true;
	}

	notification_remove_notification(pickedNotification, curNotification->dismissable, true);
}

void notification_up_rawPressed(ClickRecognizerRef recognizer, void* context)
{
	appIdle = false;
	upPressed = true;
	scroll_layer_scroll_up_click_handler(recognizer, scroll);

}
void notification_down_rawPressed(ClickRecognizerRef recognizer, void* context)
{
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
		scroll_layer_scroll_up_click_handler(recognizer, scroll);
}
void notification_down_click_proxy(ClickRecognizerRef recognizer, void* context)
{
	if (downPressed)
		scroll_layer_scroll_down_click_handler(recognizer, scroll);
}

void notification_up_double(ClickRecognizerRef recognizer, void* context)
{
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

	scroll_layer_set_content_offset(scroll, GPoint(0, 0), false);
	scroll_layer_set_content_offset(scroll, GPoint(0, 0), true);

	refresh_notification();
}

void notification_down_double(ClickRecognizerRef recognizer, void* context)
{
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

	scroll_layer_set_content_offset(scroll, GPoint(0, 0), false);
	scroll_layer_set_content_offset(scroll, GPoint(0, 0), true);

	refresh_notification();
}

void registerButtons(void* context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) notification_center_single);

	window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 500, false, (ClickHandler) notification_up_double);
	window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 500, false, (ClickHandler) notification_down_double);

	window_raw_click_subscribe(BUTTON_ID_UP, (ClickHandler) notification_up_rawPressed, (ClickHandler) notification_up_rawReleased, NULL);
	window_raw_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) notification_down_rawPressed, (ClickHandler) notification_down_rawReleased, NULL);

	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) notification_up_click_proxy);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) notification_down_click_proxy);
}

void notification_sendMoreText(int32_t id, uint8_t offset)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 1);
	dict_write_int32(iterator, 1, id);
	dict_write_uint8(iterator, 2, offset);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
}

void notification_sendNextNotification()
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 2);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	stopBusyAfterSend = true;
}

void notification_newNotification(DictionaryIterator *received)
{
	set_busy_indicator(true);


	int32_t id = dict_find(received, 1)->value->int32;
	textSize = dict_find(received, 2)->value->uint8;

	uint8_t* configBytes = dict_find(received, 2)->value->data;

	textSize = configBytes[0];

	uint8_t flags = configBytes[1];
	bool inList = (flags & 0x02) != 0;
	bool autoSwitch = (flags & 0x04) != 0;
	vibratePeriodically = (flags & 0x08) != 0;
	bool vibrateLonger = (flags & 0x10) != 0;

	enableCloseWindow = (flags & 0x20) != 0;
	if (!enableCloseWindow)
		close_menu_window();

	setTimeout = dict_find(received, 3)->value->uint16;

	Notification* notification = notification_find_notification(id);
	if (notification == NULL)
	{
		notification = notification_add_notification();

		if (!inList)
		{
			if (numOfNotifications == 1 && vibrateLonger)
				vibes_long_pulse();
			else
				vibes_short_pulse();

			appIdle = true;
			elapsedTime = 0;
		}
	}

	notification->id = id;
	notification->inList = inList;
	notification->dismissable = (flags & 0x01) != 0;
	notification->numOfChunks = dict_find(received, 4)->value->uint8;

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
				notification_remove_notification(i, false, true);
				i--;
			}
		}
	}

	if (notification->numOfChunks == 0)
	{
		notification_sendNextNotification();
	}
	else
	{
		notification_sendMoreText(notification->id, 0);
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

	if (appIdle)
	{
		for (int i = 0; i < numOfNotifications; i++)
		{

			Notification entry = notificationData[notificationPositions[i]];
			if (entry.id != id)
				continue;

			notification_remove_notification(i, false, close);

			break;
		}
	}


	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 9);
	if (numOfNotifications < 1 && exitOnClose && close)
	{
		closeCommandSent = true;
		refresh_notification();
		scroll_layer_set_content_offset(scroll, GPoint(0,0), false);

		dict_write_uint8(iterator, 2, 0);
	}
	app_message_outbox_send();
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


void notification_second_tick()
{
	if (closeCommandSent)
		return;

	elapsedTime++;

	if (appIdle && setTimeout > 0 && setTimeout < elapsedTime)
	{
		closeApp();
		return;
	}

	if (vibratePeriodically && appIdle && elapsedTime > 0 && elapsedTime % 10 == 0 && !notificationData[notificationPositions[pickedNotification]].inList)
	{
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
	if (exitOnClose)
		closeApp();
	tick_timer_service_unsubscribe();
}

void notification_load(Window *window)
{
	busyIndicator = gbitmap_create_with_resource(RESOURCE_ID_INDICATOR_BUSY);
}

void notification_unload(Window *window)
{
	gbitmap_destroy(busyIndicator);
}

void notification_window_init(bool liveNotification)
{
	exitOnClose = liveNotification;

	notifyWindow = window_create();

//	if (exitOnClose)
//		notifyWindow.overrides_back_button = true;


	window_set_window_handlers(notifyWindow, (WindowHandlers) {
		.appear = (WindowHandler)notification_appears,
		.disappear = (WindowHandler) notification_disappears,
		.load = (WindowHandler) notification_load,
		.unload = (WindowHandler) notification_unload
	});


	closeCommandSent = false;
	numOfNotifications = 0;
	for (int i = 0; i < 8; i++)
	{
		notificationDataUsed[i] = false;
	}

	window_set_click_config_provider(notifyWindow, (ClickConfigProvider) registerButtons);
	//registerButtons();

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
	scroll_layer_add_child(scroll, (Layer*) title);

	subTitle = text_layer_create(GRect(2, 18, 144 - 4, 16));
	text_layer_set_font(subTitle, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_overflow_mode(subTitle, GTextOverflowModeWordWrap);
	scroll_layer_add_child(scroll, (Layer*) subTitle);

	text = text_layer_create(GRect(2, 18 + 16, 144 - 4, 16));
	text_layer_set_font(text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_overflow_mode(text, GTextOverflowModeWordWrap);
	scroll_layer_add_child(scroll, (Layer*) text);

	window_set_fullscreen(notifyWindow, true);
	window_stack_push(notifyWindow, false);

}
