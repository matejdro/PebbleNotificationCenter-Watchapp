#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "CircularBuffer.h"

#define LIST_STORAGE_SIZE 6

static Window* window;

static uint16_t numEntries = 0;
static int16_t pickedEntry = -1;
static bool firstListRequest = true;

typedef struct
{
	char* title;
	char* subtitle;
	uint8_t type;
	char* date;

	#ifndef PBL_LOW_MEMORY
		uint16_t iconSize;
		uint8_t* iconData;
		GBitmap* icon;
    #endif
} NotificationListEntry;

static CircularBuffer* notifications;

static MenuLayer* menuLayer;

static StatusBarLayer* statusBar;

static GBitmap* normalNotification;
static GBitmap* ongoingNotification;

static void allocateData(void) {
	notifications = cb_create(sizeof(NotificationListEntry), LIST_STORAGE_SIZE);

	NotificationListEntry* notificationsBuffer = (NotificationListEntry*) notifications->data;

	for (int i = 0; i < LIST_STORAGE_SIZE; i++) {
		notificationsBuffer[i].title = malloc(sizeof(char) * 21);
		notificationsBuffer[i].subtitle = malloc(sizeof(char) * 21);
		notificationsBuffer[i].date = malloc(sizeof(char) * 21);

		*notificationsBuffer[i].title = 0;
		*notificationsBuffer[i].subtitle = 0;
		*notificationsBuffer[i].date = 0;

		#ifndef PBL_LOW_MEMORY
			notificationsBuffer[i].iconData = NULL;
			notificationsBuffer[i].icon = NULL;
		#endif
	}
}

static void freeData(void) {
	NotificationListEntry* notificationsBuffer = (NotificationListEntry*) notifications->data;

	for (int i = 0; i < LIST_STORAGE_SIZE; i++) {
		free(notificationsBuffer[i].title);
		free(notificationsBuffer[i].subtitle);
		free(notificationsBuffer[i].date);

		#ifndef PBL_LOW_MEMORY
			free(notificationsBuffer[i].iconData);
			if (notificationsBuffer[i].icon != NULL)
				gbitmap_destroy(notificationsBuffer[i].icon);
        #endif
	}

	cb_destroy(notifications);
}

static void requestNotification(uint16_t pos) {
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 2);
	dict_write_uint8(iterator, 1, 0);
	dict_write_uint16(iterator, 2, pos);

	dict_write_uint8(iterator, 3, firstListRequest ? 1 : 0);
	firstListRequest = false;

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();
}

static void sendpickedEntry(int16_t pos) {
	DictionaryIterator *iterator;
	AppMessageResult result = app_message_outbox_begin(&iterator);
	if (result != APP_MSG_OK) {
		pickedEntry = pos;
		return;
	}

	dict_write_uint8(iterator, 0, 2);
	dict_write_uint8(iterator, 1, 1);
	dict_write_uint16(iterator, 2, pos);

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();
}

static void requestAdditionalEntries(void) {
	// Do not request more list entries if user already picked something
	if (pickedEntry >= 0)
	{
		return;
	}


	uint8_t filledDown = cb_getNumOfLoadedSpacesDownFromCenter(notifications, numEntries);
	uint8_t filledUp = cb_getNumOfLoadedSpacesUpFromCenter(notifications);

	if (filledDown < LIST_STORAGE_SIZE / 2 && filledDown <= filledUp) {
		uint16_t startingIndex = notifications->centerIndex + filledDown;
		requestNotification(startingIndex);
	} else if (filledUp < LIST_STORAGE_SIZE / 2) {
		uint16_t startingIndex = notifications->centerIndex - filledUp;
		requestNotification(startingIndex);
	}
}

static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *me,
		uint16_t section_index, void *data) {
	return numEntries;
}

static int16_t menu_get_row_height_callback(MenuLayer *me,
		MenuIndex *cell_index, void *data) {
	return 56;
}

static int16_t menu_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 1;
}

static void menu_pos_changed(struct MenuLayer *menu_layer, MenuIndex new_index,
		MenuIndex old_index, void *callback_context) {
	cb_shift(notifications, new_index.row);
	requestAdditionalEntries();
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
		MenuIndex *cell_index, void *data) {
	graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorBlack, menu_cell_layer_is_highlighted(cell_layer) ? GColorWhite : GColorBlack));
	graphics_context_set_compositing_mode(ctx, GCompOpSet);

	GRect bounds = layer_get_frame(cell_layer);

	uint8_t xOffset = PBL_IF_ROUND_ELSE(20, 1);

	NotificationListEntry* listEntry = cb_getEntry(notifications, cell_index->row);
	if (listEntry == NULL)
		return;

	graphics_draw_text(ctx, listEntry->title,
			fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
			GRect(33 + xOffset, 0, bounds.size.w - 35 - xOffset, 21), GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, listEntry->subtitle,
			fonts_get_system_font(FONT_KEY_GOTHIC_14),
			GRect(33 + xOffset, 21, bounds.size.w - 35 - xOffset, 17), GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, listEntry->date,
			fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
			GRect(33 + xOffset, 39, bounds.size.w - 35 - xOffset, 18), GTextOverflowModeTrailingEllipsis,
			GTextAlignmentLeft, NULL);

	GBitmap* image = NULL;

	#ifndef PBL_LOW_MEMORY
    	image = listEntry->icon;
	#endif

	if (image == NULL)
	{
		switch (listEntry->type) {
			case 1:
				image = ongoingNotification;
				break;
			default:
				image = normalNotification;
				break;
		}
	}


	graphics_draw_bitmap_in_rect(ctx, image, GRect(xOffset, 14, 31, 31));
}

static void menu_select_callback(MenuLayer *me, MenuIndex *cell_index,
		void *data) {
	sendpickedEntry(cell_index->row);
}

static void receivedEntries(DictionaryIterator* data) {
	uint16_t offset = dict_find(data, 2)->value->uint16;
	numEntries = dict_find(data, 3)->value->uint16;

	NotificationListEntry* listEntry = cb_getEntryForFilling(notifications, offset);
	if (listEntry == NULL)
		return;

	listEntry->type = dict_find(data, 4)->value->uint8;
	strcpy(listEntry->title, dict_find(data, 5)->value->cstring);
	strcpy(listEntry->subtitle, dict_find(data, 6)->value->cstring);
	strcpy(listEntry->date, dict_find(data, 7)->value->cstring);

	#ifndef PBL_LOW_MEMORY
		free(listEntry->iconData);
		listEntry->iconData = NULL;

		if (listEntry->icon != NULL)
		{
			gbitmap_destroy(listEntry->icon);
			listEntry->icon = NULL;
		}

		listEntry->iconSize = dict_find(data, 8)->value->uint16;
		if (listEntry->iconSize != 0)
		{
			listEntry->iconData = malloc(listEntry->iconSize);
			memcpy(listEntry->iconData, dict_find(data, 9)->value->data, listEntry->iconSize);
			listEntry->icon = gbitmap_create_from_png_data(listEntry->iconData, listEntry->iconSize);
		}
	#endif


	menu_layer_reload_data(menuLayer);

	requestAdditionalEntries();
}

void list_window_data_sent(void) {
	if (pickedEntry >= 0) {
		sendpickedEntry(pickedEntry);
		return;
	}
}

void list_window_data_received(int packetId, DictionaryIterator* data) {
	switch (packetId) {
	case 0:
		receivedEntries(data);
		break;
	}
}

static void window_appear(Window* me) {
	setCurWindow(2);

	normalNotification = gbitmap_create_with_resource(RESOURCE_ID_BUBBLE);
	ongoingNotification = gbitmap_create_with_resource(RESOURCE_ID_COGWHEEL);

	Layer* topLayer = window_get_root_layer(window);
	GRect displayFrame = layer_get_frame(topLayer);

	menuLayer = menu_layer_create(GRect(0, STATUSBAR_Y_OFFSET, displayFrame.size.w, displayFrame.size.h - STATUSBAR_Y_OFFSET));

	// Set all the callbacks for the menu layer
	menu_layer_set_callbacks(menuLayer, NULL, (MenuLayerCallbacks ) {
					.get_num_sections = menu_get_num_sections_callback,
					.get_num_rows = menu_get_num_rows_callback,
					.get_cell_height = menu_get_row_height_callback,
					.get_separator_height = menu_get_separator_height_callback,
			        .draw_row = menu_draw_row_callback,
			        .select_click = menu_select_callback,
			        .selection_changed = menu_pos_changed });

	menu_layer_set_click_config_onto_window(menuLayer, window);

#ifdef PBL_COLOR
	menu_layer_set_highlight_colors(menuLayer, GColorChromeYellow, GColorBlack);
#endif

	layer_add_child(topLayer, (Layer*) menuLayer);

	statusBar = status_bar_layer_create();
	layer_add_child(topLayer, status_bar_layer_get_layer(statusBar));

	pickedEntry = -1;

	allocateData();
	menu_layer_set_selected_index(menuLayer, MenuIndex(0, notifications->centerIndex), MenuRowAlignCenter, false);

	firstListRequest = true;
	requestAdditionalEntries();
}

static void window_disappear(Window* me) {
	gbitmap_destroy(normalNotification);
	gbitmap_destroy(ongoingNotification);

	menu_layer_destroy(menuLayer);
	status_bar_layer_destroy(statusBar);

	freeData();

}

static void window_load(Window *me) {
	numEntries = 0;
}

static void window_unload(Window *me) {
	window_destroy(me);
}

void list_window_init(void) {
	window = window_create();

	window_set_window_handlers(window, (WindowHandlers ) { .appear =
					window_appear, .load = window_load, .unload = window_unload,
					.disappear = window_disappear

			});


	window_stack_push(window, false /* Animated */);

}

