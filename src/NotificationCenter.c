#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationsWindow.h"
#include "MainMenu.h"
#include "NotificationList.h"

static const uint16_t PROTOCOL_VERSION = 11;

uint8_t curWindow = 0;
bool gotConfig = false;

uint8_t config_titleFont;
uint8_t config_subtitleFont;
uint8_t config_bodyFont;
uint16_t config_timeout;
uint8_t config_vibratePeriodically;
bool config_autoSwitchNotifications;
uint8_t config_vibrateMode;
bool config_dontClose;
bool config_showActive;
bool config_lightScreen;
bool config_dontVibrateWhenCharging;
uint8_t config_shakeAction;

const char* config_getFontResource(int id)
{
	switch (id)
	{
	case 0:
		return FONT_KEY_GOTHIC_14;
	case 1:
		return FONT_KEY_GOTHIC_14_BOLD;
	case 2:
		return FONT_KEY_GOTHIC_18;
	case 3:
		return FONT_KEY_GOTHIC_18_BOLD;
	case 4:
		return FONT_KEY_GOTHIC_24;
	case 5:
		return FONT_KEY_GOTHIC_24_BOLD;
	case 6:
		return FONT_KEY_GOTHIC_28;
	case 7:
		return FONT_KEY_GOTHIC_28_BOLD;
	case 8:
		return FONT_KEY_BITHAM_30_BLACK;
	case 9:
		return FONT_KEY_BITHAM_42_BOLD;
	case 10:
		return FONT_KEY_BITHAM_42_LIGHT;
	case 11:
		return FONT_KEY_BITHAM_42_MEDIUM_NUMBERS;
	case 12:
		return FONT_KEY_BITHAM_34_MEDIUM_NUMBERS;
	case 13:
		return FONT_KEY_BITHAM_34_LIGHT_SUBSET;
	case 14:
		return FONT_KEY_BITHAM_18_LIGHT_SUBSET;
	case 15:
		return FONT_KEY_ROBOTO_CONDENSED_21;
	case 16:
		return FONT_KEY_ROBOTO_BOLD_SUBSET_49;
	case 17:
		return FONT_KEY_DROID_SERIF_28_BOLD;
	}

	return FONT_KEY_GOTHIC_14;
}

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

void received_config(DictionaryIterator *received)
{
	uint8_t* data = dict_find(received, 1)->value->data;

	uint16_t supportedVersion = (data[8] << 8) | (data[9]);
	if (supportedVersion > PROTOCOL_VERSION)
	{
		show_old_watchapp();
		return;
	}
	else if (supportedVersion < PROTOCOL_VERSION)
	{
		show_old_android();
		return;
	}

	config_titleFont = data[0];
	config_subtitleFont = data[1];
	config_bodyFont = data[2];
	config_timeout = (data[3] << 8) | (data[4]);
	config_vibratePeriodically = data[5];
	config_vibrateMode = data[6];
	config_autoSwitchNotifications = (data[7] & 0x01) != 0;
	config_dontClose = (data[7] & 0x02) != 0;
	config_showActive = (data[7] & 0x04) != 0;
	config_lightScreen = (data[7] & 0x10) != 0;
	config_dontVibrateWhenCharging = (data[7] & 0x20) != 0;
	config_shakeAction = data[10];

	gotConfig = true;

	bool notificationWaiting = (data[7] & 0x08) != 0;
	if (notificationWaiting || !config_showActive)
	{
		app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

		DictionaryIterator *iterator;
		app_message_outbox_begin(&iterator);
		dict_write_uint8(iterator, 0, 10);
		app_message_outbox_send();
	}
	else
	{
		show_menu();
	}

}

void received_data(DictionaryIterator *received, void *context) {
	uint8_t packetId = dict_find(received, 0)->value->uint8;

	if (packetId == 3)
	{
		received_config(received);
		return;
	}
	else if (!gotConfig)
		return;

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

void comm_failed(DictionaryIterator *received, AppMessageResult reason, void *context)
{
	switch (curWindow)
	{
	case 0:
		menu_comm_failed(received, reason, context);
		break;
	}
}

int main(void) {
	app_message_register_inbox_received(received_data);
	app_message_register_outbox_sent(data_sent);
	app_message_register_outbox_failed(comm_failed);
	app_message_open(124, 50);

	switchWindow(0);

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 0);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	app_event_loop();

	window_stack_pop_all(false);
	return 0;
}
