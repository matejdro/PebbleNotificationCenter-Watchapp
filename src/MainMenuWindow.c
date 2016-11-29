#include <pebble.h>
#include <pebble_fonts.h>
#include <pebble-activity-indicator-layer/activity-indicator-layer.h>
#include "NotificationCenter.h"
#include "NotificationsWindow/NotificationsWindow.h"

static Window* window;

static SimpleMenuItem notificationSectionItems[2] = {};
static SimpleMenuItem settingsSectionItems[3] = {};
static SimpleMenuSection mainMenuSections[2] = {};

static GBitmap* currentIcon;
static GBitmap* historyIcon;

static TextLayer* loadingLayer;
static TextLayer* quitTitle;
static TextLayer* quitText;

#ifndef PBL_LOW_MEMORY
static const uint16_t ACTIVITY_INDICATOR_SIZE = 50;
static const uint16_t ACTIVITY_INDICATOR_THICKNESS = 5;

static ActivityIndicatorLayer *loadingIndicator;
#endif

static MenuLayer* menuLayer;

static StatusBarLayer* statusBar;

static bool firstAppear = true;
static bool menuLoaded = false;
static bool historyCleared = false;

static void show_error_base(void);
static void update_settings();

static void show_loading(void)
{
	layer_set_hidden((Layer *) loadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	layer_set_hidden((Layer *) menuLayer, true);

    #ifndef PBL_LOW_MEMORY
        activity_indicator_layer_set_animating(loadingIndicator, true);
        layer_set_hidden(activity_indicator_layer_get_layer(loadingIndicator), false);
        text_layer_set_text(loadingLayer, "Notification Center");
    #else
        text_layer_set_text(loadingLayer, "Loading...");
    #endif
}

void show_old_watchapp_error(void)
{
    show_error_base();
	text_layer_set_text(loadingLayer, "Notification Center\nOutdated Watchapp \n\n Check your phone");

}

void show_old_android_error(void)
{
    show_error_base();

	text_layer_set_text(loadingLayer, "Notification Center\nUpdate Android App \n\n Open link:\n www.goo.gl/0e0h9m");
}

void show_disconnected_error(void)
{
    show_error_base();

    text_layer_set_text(loadingLayer, "Notification Center\n\nPhone is not connected.");
}

void show_quitting(void)
{
    layer_set_hidden((Layer *) loadingLayer, true);
    layer_set_hidden((Layer *) quitTitle, false);
    layer_set_hidden((Layer *) quitText, false);

#ifndef PBL_LOW_MEMORY
    activity_indicator_layer_set_animating(loadingIndicator, false);
    layer_set_hidden(activity_indicator_layer_get_layer(loadingIndicator), true);
#endif

    text_layer_set_text(quitTitle, "Press back again if app does not close in several seconds");
	text_layer_set_text(quitText, "Quitting...\n Please wait");
}

static void show_error_base(void)
{
	layer_set_hidden((Layer *) loadingLayer, false);
    layer_set_hidden((Layer *) menuLayer, true);
    layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);

	text_layer_set_font(loadingLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

#ifndef PBL_LOW_MEMORY
    activity_indicator_layer_set_animating(loadingIndicator, false);
    layer_set_hidden(activity_indicator_layer_get_layer(loadingIndicator), true);
#endif
}

void show_menu(void)
{
	menuLoaded = true;
	mainMenuSections[0].title = "Notifications";
	mainMenuSections[0].items = notificationSectionItems;
	mainMenuSections[0].num_items = config_showActive ? 2 : 1;

	mainMenuSections[1].title = "Settings";
	mainMenuSections[1].items = settingsSectionItems;
	mainMenuSections[1].num_items = 3;

	if (config_showActive)
	{
		notificationSectionItems[0].title = " Active";
		notificationSectionItems[0].icon = currentIcon;
	}

	int historyId = config_showActive ? 1 : 0;
	notificationSectionItems[historyId].title = " History";
	notificationSectionItems[historyId].icon = historyIcon;

	update_settings();

	layer_set_hidden((Layer *) loadingLayer, true);
	layer_set_hidden((Layer *) menuLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
}

static uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data) {
    return 2;
}


static uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data) {
    return mainMenuSections[section_index].num_items;
}

#ifdef PBL_ROUND
	static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
	{
			if (!menu_layer_is_index_selected(menu_layer, cell_index))
				return MENU_CELL_ROUND_UNFOCUSED_SHORT_CELL_HEIGHT;

			const SimpleMenuItem* item = &mainMenuSections[cell_index->section].items[cell_index->row];
			return item->icon == NULL ? MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_FOCUSED_TALL_CELL_HEIGHT;
	}
#else
	static int16_t menu_get_separator_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
		return 1;
	}

	static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
	{
		return MENU_CELL_BASIC_HEADER_HEIGHT;
	}

	static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context)
	{
		const SimpleMenuSection* section = &mainMenuSections[section_index];
		menu_cell_basic_header_draw(ctx, cell_layer, section->title);
	}
#endif

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    int16_t index = cell_index->row;
    int16_t section = cell_index->section;

    const SimpleMenuItem* item = &mainMenuSections[section].items[index];

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    menu_cell_basic_draw(ctx, cell_layer, item->title, item->subtitle, item->icon);
}


static void update_settings(void)
{
	if (config_disableNotifications)
	{
		settingsSectionItems[0].title = "Notifications OFF";
		settingsSectionItems[0].subtitle = "Press to enable";
	}
	else
	{
		settingsSectionItems[0].title = "Notifications ON";
		settingsSectionItems[0].subtitle = "Press to disable";
	}

	if (config_disableVibration)
	{
		settingsSectionItems[1].title = "Vibration OFF";
		settingsSectionItems[1].subtitle = "Press to enable";
	}
	else
	{
		settingsSectionItems[1].title = "Vibration ON";
		settingsSectionItems[1].subtitle = "Press to disable";
	}

	settingsSectionItems[2].title = "Clear History";
	if (historyCleared)
		settingsSectionItems[2].subtitle = "Cleared";
	else
		settingsSectionItems[2].subtitle = "Press to clear";

    menu_layer_reload_data(menuLayer);
}


static void notifications_picked(int index)
{
	//show_loading();

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	if (index == 0 && !config_showActive)
		index = 1;

	dict_write_uint8(iterator, 0, 0);
	dict_write_uint8(iterator, 1, 1);
	dict_write_uint8(iterator, 2, index);

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();
}

static void settings_picked(int index)
{
	uint8_t sendingIndex = index;
	uint8_t sendingValue = 0;

	switch (index)
	{
	case 0:
		config_disableNotifications = !config_disableNotifications;
		sendingValue = config_disableNotifications ? 1 : 0;
		break;
	case 1:
		config_disableVibration = !config_disableVibration;
		sendingValue = config_disableVibration ? 1 : 0;
		break;
	case 2:
		historyCleared = true;
		break;
	default:
		return;
	}

	update_settings();

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 0);
	dict_write_uint8(iterator, 1, 2);
	dict_write_uint8(iterator, 2, sendingIndex);
	dict_write_uint8(iterator, 3, sendingValue);

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
    if (cell_index->section == 0)
        notifications_picked(cell_index->row);
    else
        settings_picked(cell_index->row);

}

static void closing_timer(void* data)
{
	if (!closingMode)
		return;

	closeApp();
	app_timer_register(3000, closing_timer, NULL);
}

static void window_appears(Window* window)
{
	Layer* topLayer = window_get_root_layer(window);
	GRect windowBounds = layer_get_frame(topLayer);
	uint16_t windowWidth = windowBounds.size.w;
	uint16_t windowHeight = windowBounds.size.h - STATUSBAR_Y_OFFSET;

	currentIcon = gbitmap_create_with_resource(RESOURCE_ID_BUBBLE);
	historyIcon = gbitmap_create_with_resource(RESOURCE_ID_RECENT);

	loadingLayer = text_layer_create(GRect(0, STATUSBAR_Y_OFFSET, windowWidth, windowHeight));
	text_layer_set_text_alignment(loadingLayer, GTextAlignmentCenter);
	text_layer_set_font(loadingLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(topLayer, (Layer*) loadingLayer);

    #ifndef PBL_LOW_MEMORY
        loadingIndicator = activity_indicator_layer_create(GRect(windowWidth / 2 - ACTIVITY_INDICATOR_SIZE / 2, windowHeight / 2 - ACTIVITY_INDICATOR_SIZE / 2 + STATUSBAR_Y_OFFSET, ACTIVITY_INDICATOR_SIZE, ACTIVITY_INDICATOR_SIZE));
        activity_indicator_layer_set_thickness(loadingIndicator, ACTIVITY_INDICATOR_THICKNESS);

        layer_add_child(topLayer, (Layer *)loadingIndicator);
    #endif

    quitTitle = text_layer_create(GRect(0, 70 + STATUSBAR_Y_OFFSET, windowWidth, 50));
	text_layer_set_text_alignment(quitTitle, GTextAlignmentCenter);
	layer_add_child(topLayer, (Layer*) quitTitle);

	quitText = text_layer_create(GRect(0, 10 + STATUSBAR_Y_OFFSET, windowWidth, 50));
	text_layer_set_text_alignment(quitText, GTextAlignmentCenter);
	text_layer_set_font(quitText, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(topLayer, (Layer*) quitText);

    menuLayer = menu_layer_create(GRect(0, STATUSBAR_Y_OFFSET, windowWidth, windowHeight));

    // Set all the callbacks for the menu layer
    menu_layer_set_callbacks(menuLayer, NULL, (MenuLayerCallbacks){
            .get_num_sections = menu_get_num_sections_callback,
            .get_num_rows = menu_get_num_rows_callback,
            .select_click = menu_select_callback,
			.draw_row = menu_draw_row_callback,
           #ifdef PBL_RECT
			.get_separator_height = menu_get_separator_height_callback,
			.get_header_height = menu_get_header_height_callback,
			.draw_header = menu_draw_header_callback,
           #else
			.get_cell_height = menu_get_cell_height_callback,
		   #endif
	});

	#ifdef PBL_PLATFORM_CHALK
		menu_layer_set_center_focused(menuLayer, true);
	#endif

	layer_set_hidden((Layer *) menuLayer, true);

#ifdef PBL_COLOR
    menu_layer_set_highlight_colors(menuLayer, GColorChromeYellow, GColorBlack);
#endif

    menu_layer_set_click_config_onto_window(menuLayer, window);
    layer_add_child(topLayer, menu_layer_get_layer(menuLayer));
	statusBar = status_bar_layer_create();
	layer_add_child(topLayer, status_bar_layer_get_layer(statusBar));

	historyCleared = false;

	setCurWindow(0);
	if (menuLoaded && !closingMode)
		show_menu();
	else
		show_quitting();

	app_timer_register(3000, closing_timer, NULL);
}

static void window_disappears(Window* me)
{
	gbitmap_destroy(currentIcon);
	gbitmap_destroy(historyIcon);

	text_layer_destroy(loadingLayer);
	text_layer_destroy(quitTitle);
	text_layer_destroy(quitText);

    menu_layer_destroy(menuLayer);
	status_bar_layer_destroy(statusBar);

    #ifndef PBL_LOW_MEMORY
        activity_indicator_layer_destroy(loadingIndicator);
    #endif

    closingMode = false;
}

static void window_load(Window *me) {
	firstAppear = false;
}


static void window_unload(Window* me)
{
	window_destroy(me);
}

void main_menu_close(void)
{
	if (window != NULL)
	{
		window_stack_remove(window, false);
		window = NULL;
	}
}

void main_menu_init(void)
{
	window = window_create();

	window_set_window_handlers(window, (WindowHandlers){
		.load = window_load,
		.unload = window_unload,
		.appear = window_appears,
		.disappear = window_disappears,
	});

	window_stack_push(window, true /* Animated */);

    if (connection_service_peek_pebble_app_connection())
	    show_loading();
    else
        show_disconnected_error();
}

