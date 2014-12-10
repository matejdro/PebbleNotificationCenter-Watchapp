#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "NotificationsWindow.h"
#include "NotificationList.h"

Window* menuWindow;

SimpleMenuItem notificationSectionItems[2] = {};
SimpleMenuItem settingsSectionItems[2] = {};
SimpleMenuSection mainMenuSections[2] = {};

GBitmap* currentIcon;
GBitmap* historyIcon;

TextLayer* menuLoadingLayer;

TextLayer* quitTitle;
TextLayer* quitText;

SimpleMenuLayer* menuLayer;

InverterLayer* inverterLayer;

bool firstAppear = true;
bool menuLoaded = false;

void show_loading()
{
	layer_set_hidden((Layer *) menuLoadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);
}

void show_update_dialog()
{
	layer_set_hidden((Layer *) menuLoadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);

	text_layer_set_font(menuLoadingLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
}

void show_old_watchapp()
{
	show_update_dialog();
	text_layer_set_text(menuLoadingLayer, "Notification Center\nOutdated Watchapp \n\n Check your phone");

}

void show_old_android()
{
	show_update_dialog();

	text_layer_set_text(menuLoadingLayer, "Notification Center\nUpdate Android App \n\n Open link:\n www.goo.gl/0e0h9m");

}

void show_quit()
{
	layer_set_hidden((Layer *) menuLoadingLayer, true);
	layer_set_hidden((Layer *) quitTitle, false);
	layer_set_hidden((Layer *) quitText, false);
}

inline void reload_menu()
{
	Layer* topLayer = window_get_root_layer(menuWindow);


	if (menuLayer != NULL)
	{
		layer_remove_from_parent((Layer *) menuLayer);
		simple_menu_layer_destroy(menuLayer);
	}

	menuLayer = simple_menu_layer_create(GRect(0, 0, 144, 152), menuWindow, mainMenuSections, 2, NULL);

	layer_add_child(topLayer, (Layer *) menuLayer);

	if (inverterLayer != NULL)
	{
		layer_remove_from_parent((Layer*) inverterLayer);
		inverter_layer_destroy(inverterLayer);
		inverterLayer = inverter_layer_create(layer_get_frame(topLayer));
		layer_add_child(topLayer, (Layer*) inverterLayer);
	}
}

void update_notifications_enabled_setting()
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


void notifications_picked(int index, void* context)
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

void settings_picked(int index, void* context)
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

	update_notifications_enabled_setting();
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

void show_menu()
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

	update_notifications_enabled_setting();
	settingsSectionItems[0].callback = settings_picked;
	settingsSectionItems[1].callback = settings_picked;

	reload_menu();

	layer_set_hidden((Layer *) menuLoadingLayer, true);
	layer_set_hidden((Layer *) menuLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
}

void window_unload(Window* me)
{
	gbitmap_destroy(currentIcon);
	gbitmap_destroy(historyIcon);

	text_layer_destroy(menuLoadingLayer);
	text_layer_destroy(quitTitle);
	text_layer_destroy(quitText);

	if (inverterLayer != NULL)
		inverter_layer_destroy(inverterLayer);

	window_destroy(me);
}

void window_load(Window *me) {
	currentIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON);
	historyIcon = gbitmap_create_with_resource(RESOURCE_ID_RECENT);

	Layer* topLayer = window_get_root_layer(menuWindow);

	menuLoadingLayer = text_layer_create(GRect(0, 0, 144, 168 - 16));
	text_layer_set_text_alignment(menuLoadingLayer, GTextAlignmentCenter);
	text_layer_set_text(menuLoadingLayer, "Loading...");
	text_layer_set_font(menuLoadingLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(topLayer, (Layer*) menuLoadingLayer);

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
}

void close_menu_window()
{
	if (menuWindow != NULL)
	{
		window_stack_remove(menuWindow, false);
		//menuWindow = NULL;
	}
}

void closing_timer(void* data)
{
	closeApp();
	app_timer_register(5000, closing_timer, NULL);
}

void menu_appears(Window* window)
{
	firstAppear = false;

	setCurWindow(0);
	if (menuLoaded && !closingMode)
		show_menu();

	if (closingMode)
	{
		app_timer_register(3000, closing_timer, NULL);
	}
}

void init_menu_window()
{
	menuWindow = window_create();

	window_set_window_handlers(menuWindow, (WindowHandlers){
		.load = window_load,
		.unload = window_unload,
		.appear = menu_appears,
	});

	window_stack_push(menuWindow, true /* Animated */);

	show_loading();
}

