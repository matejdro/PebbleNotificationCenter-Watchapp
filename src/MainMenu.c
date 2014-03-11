#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"
#include "NotificationsWindow.h"
#include "NotificationList.h"

Window* menuWindow;

SimpleMenuItem mainMenuItems[2] = {};
SimpleMenuSection mainMenuSection[1] = {};

GBitmap* currentIcon;
GBitmap* historyIcon;

TextLayer* menuLoadingLayer;

TextLayer* quitTitle;
TextLayer* quitText;


SimpleMenuLayer* menuLayer;

void show_loading()
{
	layer_set_hidden((Layer *) menuLoadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);
}

void show_old_watchapp()
{
	layer_set_hidden((Layer *) menuLoadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);

	text_layer_set_text(menuLoadingLayer, "Please update Notification Center watch app");

}

void show_old_android()
{
	layer_set_hidden((Layer *) menuLoadingLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
	if (menuLayer != NULL) layer_set_hidden((Layer *) menuLayer, true);

	text_layer_set_text(menuLoadingLayer, "Please update Notification Center android app");

}

void show_quit()
{
	layer_set_hidden((Layer *) menuLoadingLayer, true);
	layer_set_hidden((Layer *) quitTitle, false);
	layer_set_hidden((Layer *) quitText, false);
}

void menu_picked(int index, void* context)
{
	show_loading();

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 6);
	dict_write_uint8(iterator, 1, index);

	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
}

void show_menu()
{
	mainMenuSection[0].items = mainMenuItems;
	mainMenuSection[0].num_items = 2;

	mainMenuItems[0].title = "Active";
	mainMenuItems[0].icon = currentIcon;
	mainMenuItems[0].callback = menu_picked;

	mainMenuItems[1].title = "History";
	mainMenuItems[1].icon = historyIcon;
	mainMenuItems[1].callback = menu_picked;

	Layer* topLayer = window_get_root_layer(menuWindow);

	if (menuLayer != NULL) layer_remove_from_parent((Layer *) menuLayer);
	menuLayer = simple_menu_layer_create(GRect(0, 0, 144, 156), menuWindow, mainMenuSection, 1, NULL);
	layer_add_child(topLayer, (Layer *) menuLayer);

	layer_set_hidden((Layer *) menuLoadingLayer, true);
	layer_set_hidden((Layer *) menuLayer, false);
	layer_set_hidden((Layer *) quitTitle, true);
	layer_set_hidden((Layer *) quitText, true);
}



void menu_data_received(int packetId, DictionaryIterator* data)
{
	switch (packetId)
	{
	case 0:
		notification_window_init(true);
		notification_received_data(packetId, data);

		if (config_dontClose)
			show_quit();
		else
			window_stack_remove(menuWindow, false);

		break;
	case 2:
		window_stack_pop(true);
		init_notification_list_window();
		list_data_received(packetId, data);

		break;
	}

}

void window_unload(Window* me)
{
	gbitmap_destroy(currentIcon);
	gbitmap_destroy(historyIcon);

	text_layer_destroy(menuLoadingLayer);
	text_layer_destroy(quitTitle);
	text_layer_destroy(quitText);

	window_destroy(me);
	me = NULL;
}

void window_load(Window *me) {
	currentIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON);
	historyIcon = gbitmap_create_with_resource(RESOURCE_ID_RECENT);

	Layer* topLayer = window_get_root_layer(menuWindow);

	menuLoadingLayer = text_layer_create(GRect(10, 10, 144 - 10, 168 - 16));
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


	setCurWindow(0);
}



void init_menu_window()
{
	menuWindow = window_create();

	window_set_window_handlers(menuWindow, (WindowHandlers){
		.appear = window_load,
		.unload = window_unload
	});

	window_stack_push(menuWindow, true /* Animated */);

	show_loading();
}

