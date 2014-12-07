#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "MainMenu.h"

typedef struct
{
	int32_t id;
	bool inList;
	bool scrollToEnd;
	bool showMenuOnSelectPress;
	bool showMenuOnSelectHold;
	uint8_t numOfActions;
	char title[31];
	char subTitle[31];
	char text[901];

} Notification;


uint32_t elapsedTime = 0;
bool appIdle = true;
bool vibrating = false;

uint16_t periodicVibrationPeriod = 0;

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
uint8_t skippedUpPresses = 0;
uint8_t skippedDownPresses = 0;

TextLayer* title;
TextLayer* subTitle;
TextLayer* text;

Layer* menuBackground;
MenuLayer* actionsMenu;
bool actionsMenuDisplayed = false;
char actions[20][20];
int8_t actionPicked = -1;

void registerButtons(void* context);
static void menu_show();
static void menu_hide();

void refresh_notification()
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

void scroll_to_start()
{
	int16_t scrollTo = 0;

	Notification* notification = &notificationData[notificationPositions[pickedNotification]];;
	if (notification != NULL && notification->scrollToEnd)
		scrollTo = -scroll_layer_get_content_size(scroll).h;

	scroll_layer_set_content_offset(scroll, GPoint(0, scrollTo), false);
	scroll_layer_set_content_offset(scroll, GPoint(0, scrollTo), true);
}

void scroll_by(int16_t amount)
{
	GSize size = scroll_layer_get_content_size(scroll);
	GPoint point = scroll_layer_get_content_offset(scroll);
	point.y += amount;
	if (point.y < -size.h)
		point.y = -size.h;

	scroll_layer_set_content_offset(scroll, point, true);
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
			menu_hide();
			refresh_notification();
			scroll_to_start();
		}

	}
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

static void actions_init()
{
	for (int i = 0; i < 20; i++)
	{
		actions[i][0] = 0;
	}
}

static void menu_show()
{
	actions_init();
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

static void menu_up()
{
	uint8_t numOfActions = notificationData[notificationPositions[pickedNotification]].numOfActions;

	MenuIndex index = menu_layer_get_selected_index(actionsMenu);
	if (index.row == 0)
	{
		menu_layer_set_selected_index(actionsMenu, MenuIndex(0,numOfActions - 1), MenuRowAlignCenter, true);
		return;
	}

	menu_layer_set_selected_next(actionsMenu, true, MenuRowAlignCenter, true);
}

static void menu_down()
{
	uint8_t numOfActions = notificationData[notificationPositions[pickedNotification]].numOfActions;

	MenuIndex index = menu_layer_get_selected_index(actionsMenu);
	if (index.row == numOfActions - 1)
	{
		menu_layer_set_selected_index(actionsMenu, MenuIndex(0, 0), MenuRowAlignCenter, true);
		return;
	}

	menu_layer_set_selected_next(actionsMenu, false, MenuRowAlignCenter, true);
}

void notification_action(int action)
{
	actionPicked = -1;

	DictionaryIterator *iterator;
	AppMessageResult result = app_message_outbox_begin(&iterator);
	if (result != APP_MSG_OK)
	{
		actionPicked = action;
		return;
	}

	dict_write_uint8(iterator, 0, 4);
	dict_write_uint8(iterator, 1, 2);
	dict_write_uint8(iterator, 2, action);

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

	set_busy_indicator(true);
	menu_hide();
}

void notification_sendSelectAction(int32_t notificationId, bool hold)
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

void notification_dismiss(int32_t notificationId)
{
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 4);
	dict_write_uint8(iterator, 1, 1);

	dict_write_int32(iterator, 2, notificationId);
	app_message_outbox_send();

	set_busy_indicator(true);
}

void notification_back_single(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
	{
		menu_hide();
	}
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
		notification_action(menu_layer_get_selected_index(actionsMenu).row);
	}
	else
	{
		if (busy)
			return;

		if (curNotification->showMenuOnSelectPress)
			menu_show();

		notification_sendSelectAction(curNotification->id, false);
	}
}

void notification_center_hold(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
		return;

	Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
	if (curNotification == NULL)
		return;

	if (busy)
		return;

	if (curNotification->showMenuOnSelectHold)
		menu_show();

	notification_sendSelectAction(curNotification->id, true);
}

void notification_up_rawPressed(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
	{
		menu_up();
	}
	else
	{
		scroll_layer_scroll_up_click_handler(recognizer, scroll);
	}

	appIdle = false;
	upPressed = true;
	skippedUpPresses = 0;

}
void notification_down_rawPressed(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
	{
		menu_down();
	}
	else
	{
		scroll_layer_scroll_down_click_handler(recognizer, scroll);
	}

	appIdle = false;
	downPressed = true;
	skippedDownPresses = 0;
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
			menu_up();
			return;
		}

		//Scroll layer can't handle being pressed every 50ms, so we only use 1/2 of the presses.
		if (skippedUpPresses == 1 )
		{
			scroll_by(50);
			skippedUpPresses = 0;
		}
		else
		{
			skippedUpPresses = 1;
		}
	}
}
void notification_down_click_proxy(ClickRecognizerRef recognizer, void* context)
{
	if (downPressed)
	{
		if (actionsMenuDisplayed)
		{
			menu_down();
			return;
		}


		//Scroll layer can't handle being pressed every 50ms, so we only use 1/2 of the presses.
		if (skippedDownPresses == 1 )
		{
			scroll_by(-50);
		}
		else
		{
			skippedDownPresses = 1;
		}
	}
}

void notification_up_double(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
		return;

	if (pickedNotification == 0)
	{
		Notification data = notificationData[notificationPositions[pickedNotification]];
		if (data.inList && !busy)
		{
			DictionaryIterator *iterator;
			app_message_outbox_begin(&iterator);
			dict_write_uint8(iterator, 0, 2);
			dict_write_uint8(iterator, 1, 2);
			dict_write_int8(iterator, 2, -1);
			app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
			app_message_outbox_send();

			set_busy_indicator(true);
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
	scroll_to_start();
}

void notification_down_double(ClickRecognizerRef recognizer, void* context)
{
	if (actionsMenuDisplayed)
		return;

	if (pickedNotification == numOfNotifications - 1)
	{
		Notification data = notificationData[notificationPositions[pickedNotification]];
		if (data.inList && !busy)
		{
			DictionaryIterator *iterator;
			app_message_outbox_begin(&iterator);
			dict_write_uint8(iterator, 0, 2);
			dict_write_uint8(iterator, 1, 2);
			dict_write_int8(iterator, 2, 1);
			app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
			app_message_outbox_send();

			set_busy_indicator(true);
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
	scroll_to_start();
}

void registerButtons(void* context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) notification_center_single);
	window_single_click_subscribe(BUTTON_ID_BACK, (ClickHandler) notification_back_single);

	window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 150, false, (ClickHandler) notification_up_double);
	window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 150, false, (ClickHandler) notification_down_double);

	window_raw_click_subscribe(BUTTON_ID_UP, (ClickHandler) notification_up_rawPressed, (ClickHandler) notification_up_rawReleased, NULL);
	window_raw_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) notification_down_rawPressed, (ClickHandler) notification_down_rawReleased, NULL);

	window_long_click_subscribe(BUTTON_ID_SELECT, 500, (ClickHandler) notification_center_hold, NULL);

	window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, (ClickHandler) notification_up_click_proxy);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, (ClickHandler) notification_down_click_proxy);
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
	return notificationData[notificationPositions[pickedNotification]].numOfActions;
}


static int16_t menu_get_row_height_callback(MenuLayer *me,  MenuIndex *cell_index, void *data) {
	return 30;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, actions[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(5, 0, 144 - 20 - 10, 27), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

void vibration_stopped(void* data)
{
	vibrating = false;
}

void notification_newNotification(DictionaryIterator *received)
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

	Notification* notification = notification_find_notification(id);
	if (notification == NULL)
	{
		notification = notification_add_notification();

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
	notification->numOfActions = configBytes[3];
	strcpy(notification->title, dict_find(received, 4)->value->cstring);
	strcpy(notification->subTitle, dict_find(received, 5)->value->cstring);
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


	if (numOfNotifications == 1)
	{
		refresh_notification();
		scroll_to_start();
	}
	else if (autoSwitch && !actionsMenuDisplayed)
	{
		pickedNotification = numOfNotifications - 1;
		refresh_notification();
		scroll_to_start();
	}

	set_busy_indicator(false);

}

void notification_gotDismiss(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 2)->value->int32;
	for (int i = 0; i < numOfNotifications; i++)
	{
		Notification entry = notificationData[notificationPositions[i]];
		if (entry.id != id)
			continue;

		notification_remove_notification(i, true);

		set_busy_indicator(false);

		break;
	}
}

void notification_gotMoreText(DictionaryIterator *received)
{
	int32_t id = dict_find(received, 2)->value->int32;

	Notification* notification = notification_find_notification(id);
	if (notification == NULL)
		return;

	uint16_t length = strlen(notification->text);
	strcpy(notification->text + length, dict_find(received, 3)->value->cstring);

	if (pickedNotification == numOfNotifications - 1)
		refresh_notification();
}

void notification_gotActionListItems(DictionaryIterator *received)
{
	uint8_t firstId = dict_find(received, 2)->value->uint8;
	uint8_t* text = dict_find(received,3)->value->data;
	uint8_t numOfActions = notificationData[notificationPositions[pickedNotification]].numOfActions;

	uint8_t packetSize = numOfActions - firstId;

	if (firstId == 0)
	{
		if (!actionsMenuDisplayed)
			menu_show();

		set_busy_indicator(false);
	}

	if (packetSize > 4)
		packetSize = 4;

	for (int i = 0; i < packetSize; i++)
	{
		memcpy(actions[i + firstId], &text[i*19],  19);
	}

	if (actionsMenuDisplayed)
		menu_layer_reload_data(actionsMenu);
}



void notification_received_data(uint8_t module, uint8_t id, DictionaryIterator *received) {
	if (module == 0 && id == 1)
	{
		set_busy_indicator(false);
	}
	else if (module == 1)
	{
		if (id == 0)
			notification_newNotification(received);
		else if (id == 1)
			notification_gotMoreText(received);
	}
	else if (module == 3 && id == 0)
	{
		notification_gotDismiss(received);
	}
	else if (module == 4 && id == 0)
	{
		notification_gotActionListItems(received);
	}
}

void notification_data_sent()
{
	if (actionPicked > -1)
		notification_action(actionPicked);
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
	{
		Notification* curNotification = &notificationData[notificationPositions[pickedNotification]];
		if (curNotification == NULL)
			return;

		notification_dismiss(curNotification->id);
	}
}


void notification_second_tick()
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

void notification_appears(Window *window)
{
	setCurWindow(1);

	updateStatusClock();

	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) notification_second_tick);
}

void notification_disappears(Window *window)
{
	setCurWindow(-1);
	tick_timer_service_unsubscribe();

	if (main_noMenu && config_dontClose)
	{
		closeApp();
	}

	if (main_noMenu)
		closingMode = true;
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
	text_layer_set_overflow_mode(title, GTextOverflowModeWordWrap);
	scroll_layer_add_child(scroll, (Layer*) title);

	subTitle = text_layer_create(GRect(2, 18, 144 - 4, 16));
	text_layer_set_overflow_mode(subTitle, GTextOverflowModeWordWrap);
	scroll_layer_add_child(scroll, (Layer*) subTitle);

	text = text_layer_create(GRect(2, 18 + 16, 144 - 4, 16));
	text_layer_set_overflow_mode(text, GTextOverflowModeWordWrap);
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
	text_layer_destroy(statusClock);
	scroll_layer_destroy(scroll);
	gbitmap_destroy(busyIndicator);
	layer_destroy(menuBackground);
	menu_layer_destroy(actionsMenu);

	if (inverterLayer != NULL)
		inverter_layer_destroy(inverterLayer);

	if (config_shakeAction > 0)
		accel_tap_service_unsubscribe();

	window_destroy(window);
}

void notification_window_init()
{
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

	if (!config_dontClose)
			close_menu_window();
	else
			show_quit();
}
