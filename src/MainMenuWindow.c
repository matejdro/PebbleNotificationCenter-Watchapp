#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "NotificationsWindow.h"

static Window* window;

static SimpleMenuItem notificationSectionItems[2] = {};
static SimpleMenuItem settingsSectionItems[2] = {};
static SimpleMenuSection mainMenuSections[2] = {};

static GBitmap* currentIcon;
static GBitmap* historyIcon;

static TextLayer* loadingLayer;

static TextLayer* quitTitle;
static TextLayer* quitText;

static SimpleMenuLayer* menuLayer;

static InverterLayer* inverterLayer;

static bool firstAppear = true;
static bool menuLoaded = false;

static void show_update_dialog(void);
static void reload_menu(void);
static void update_settings();
static void settings_picked(int index, void* context);
static void notifications_picked(int index, void* context);

static void show_loading(void)
{
	layer_set_hidden((Layer *) loadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);
}

void show_old_watchapp(void)
{
	show_update_dialog();
	text_layer_set_text(loadingLayer, "Notification Center\nOutdated Watchapp \n\n Check your phone");

}

void show_old_android(void)
{
	show_update_dialog();

	text_layer_set_text(loadingLayer, "Notification Center\nUpdate Android App \n\n Open link:\n www.goo.gl/0e0h9m");

}

static void show_update_dialog(void)
{
	layer_set_hidden((Layer *) loadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);

	text_layer_set_font(loadingLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
}

void show_menu(void)
{
	menuLoaded = true;
	mainMenuSections[0].title = "Notifications";
	mainMenuSections[0].items = notificationSectionItems;
	mainMenuSections[0].num_items = config_showActive ? 2 : 1;

	mainMenuSections[1].title = "Settings";
	mainMenuSections[1].items = settingsSectionItems;
	mainMenuSections[1].num_items = 2;

	if (config_showActive)
	{
		notificationSectionItems[0].title = "Active";
		notificationSectionItems[0].icon = currentIcon;
		notificationSectionItems[0].callback = notifications_picked;
	}

	int historyId = config_showActive ? 1 : 0;
	notificationSectionItems[historyId].title = "History";
	notificationSectionItems[historyId].icon = historyIcon;
	notificationSectionItems[historyId].callback = notifications_picked;

	update_settings();
	settingsSectionItems[0].callback = settings_picked;
	settingsSectionItems[1].callback = settings_picked;

	reload_menu();

	layer_set_hidden((Layer *) loadingLayer, true);
	layer_set_hidden((Layer *) menuLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
}


static void reload_menu(void)
{
	Layer* topLayer = window_get_root_layer(window);


	if (menuLayer != NULL)
	{
		layer_remove_from_parent((Layer *) menuLayer);
		simple_menu_layer_destroy(menuLayer);
	}

	menuLayer = simple_menu_layer_create(GRect(0, 0, 144, 152), window, mainMenuSections, 2, NULL);

	layer_add_child(topLayer, (Layer *) menuLayer);

	if (inverterLayer != NULL)
	{
		layer_remove_from_parent((Layer*) inverterLayer);
		inverter_layer_destroy(inverterLayer);
		inverterLayer = inverter_layer_create(layer_get_frame(topLayer));
		layer_add_child(topLayer, (Layer*) inverterLayer);
	}
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
}


static void notifications_picked(int index, void* context)
{
	show_loading();

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

static void settings_picked(int index, void* context)
{
	uint8_t sendingIndex = index;
	uint8_t sendingValue;

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
	default:
		return;
	}

	update_settings();
	menu_layer_reload_data((MenuLayer*) menuLayer);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 0);
	dict_write_uint8(iterator, 1, 2);
	dict_write_uint8(iterator, 2, sendingIndex);
	dict_write_uint8(iterator, 3, sendingValue);

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_message_outbox_send();

}

static void closing_timer(void* data)
{
	closeApp();
	app_timer_register(5000, closing_timer, NULL);
}

static void window_appears(Window* window)
{
	Layer* topLayer = window_get_root_layer(window);

	currentIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON);
	historyIcon = gbitmap_create_with_resource(RESOURCE_ID_RECENT);

	loadingLayer = text_layer_create(GRect(0, 0, 144, 168 - 16));
	text_layer_set_text_alignment(loadingLayer, GTextAlignmentCenter);
	text_layer_set_text(loadingLayer, "Loading...");
	text_layer_set_font(loadingLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(topLayer, (Layer*) loadingLayer);

	quitTitle = text_layer_create(GRect(0, 70, 144, 50));
	text_layer_set_text_alignment(quitTitle, GTextAlignmentCenter);
	text_layer_set_text(quitTitle, "Press back again if app does not close in several seconds");
	layer_add_child(topLayer, (Layer*) quitTitle);

	quitText = text_layer_create(GRect(0, 10, 144, 50));
	text_layer_set_text_alignment(quitText, GTextAlignmentCenter);
	text_layer_set_text(quitText, "Quitting...\n Please wait");
	text_layer_set_font(quitText, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(topLayer, (Layer*) quitText);

	if (config_invertColors)
	{
		inverterLayer = inverter_layer_create(layer_get_frame(topLayer));
		layer_add_child(topLayer, (Layer*) inverterLayer);
	}

	setCurWindow(0);
	if (menuLoaded && !closingMode)
		show_menu();

	if (closingMode)
	{
		app_timer_register(3000, closing_timer, NULL);
	}
}

static void window_disappears(Window* me)
{
	gbitmap_destroy(currentIcon);
	gbitmap_destroy(historyIcon);

	text_layer_destroy(loadingLayer);
	text_layer_destroy(quitTitle);
	text_layer_destroy(quitText);

	if (inverterLayer != NULL)
		inverter_layer_destroy(inverterLayer);
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

	show_loading();
}

