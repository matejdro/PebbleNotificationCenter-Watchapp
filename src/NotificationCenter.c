#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationsWindow.h"
#include "MainMenu.h"
#include "NotificationList.h"

uint8_t curWindow = 0;
bool gotNotification = false;

uint8_t getCurWindow()
{
	return curWindow;
}

void setCurWindow(uint8_t newWindow)
{
	curWindow = newWindow;
}

void switchWindow(uint8_t newWindow)
{
	switch(newWindow)
	{
	case 0:
		curWindow = 0;
		init_menu_window();
		break;
	case 1:
		curWindow = 1;
		notification_window_init(false);
		break;
	}
}

void received_data(DictionaryIterator *received, void *context) {
	gotNotification = true;

	uint8_t packetId = dict_find(received, 0)->value->uint8;

	if (packetId == 0 && curWindow > 1)
	{
		switchWindow(1);
	}

	switch (curWindow)
	{
	case 0:
		menu_data_received(packetId, received);
		break;
	case 1:
		notification_received_data(packetId, received);
		break;
	case 2:
		list_data_received(packetId, received);
		break;
	}

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
}

void data_sent(DictionaryIterator *received, void *context)
{
	switch (curWindow)
	{
	case 1:
		notification_data_sent(received, context);
		break;
	}
}

void timerTriggered(void* context)
{
	if (!gotNotification)
	{
		DictionaryIterator *iterator;
		app_message_outbox_begin(&iterator);
		dict_write_uint8(iterator, 0, 0);
		app_message_outbox_send();

		app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	}
}


int main(void) {
	app_message_register_inbox_received(received_data);
	app_message_register_outbox_sent(data_sent);
	app_message_open(124, 50);

	switchWindow(0);

	app_timer_register(300, timerTriggered, NULL);

	app_event_loop();
	return 0;
}
