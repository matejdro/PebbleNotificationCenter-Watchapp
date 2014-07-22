#include <pebble.h>
#include <pebble_fonts.h>
#include "NotificationCenter.h"

Window* listWindow;

uint16_t numEntries = 10;

int8_t arrayCenterPos = 0;
int16_t centerIndex = 0;

int16_t pickedEntry = -1;
uint8_t pickedMode = 0;

bool ending = false;
char titles[21][21] = {};
char subtitles[21][21] = {};
uint8_t types[21] = {};
char dates[21][21] = {};

MenuLayer* listMenuLayer;

GBitmap* normalNotification;
GBitmap* ongoingNotification;

int8_t convertToArrayPos(uint16_t index)
{
	int16_t indexDiff = index - centerIndex;
	if (indexDiff > 10 || indexDiff < -10)
		return -1;

	int8_t arrayPos = arrayCenterPos + indexDiff;
	if (arrayPos < 0)
		arrayPos += 21;
	if (arrayPos > 20)
		arrayPos -= 21;

	return arrayPos;
}

char* getTitle(uint16_t index)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return "";

	return titles[arrayPos];
}

void setTitle(uint16_t index, char *name)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	strcpy(titles[arrayPos],name);
}

char* getSubtitle(uint16_t index)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return "";

	return subtitles[arrayPos];
}

void setSubtitle(uint16_t index, char *name)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	strcpy(subtitles[arrayPos],name);
}

uint8_t getType(uint16_t index)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return 0;

	return types[arrayPos];
}

void setType(uint16_t index, uint8_t type)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	types[arrayPos] = type;
}

char* getDate(uint16_t index)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return "";

	return dates[arrayPos];
}

void setDate(uint16_t index, char *name)
{
	int8_t arrayPos = convertToArrayPos(index);
	if (arrayPos < 0)
		return;

	strcpy(dates[arrayPos],name);
}

void shiftArray(int newIndex)
{
	int8_t clearIndex;

	int16_t diff = newIndex - centerIndex;
	if (diff == 0)
		return;

	centerIndex+= diff;
	arrayCenterPos+= diff;

	if (diff > 0)
	{
		if (arrayCenterPos > 20)
			arrayCenterPos -= 21;

		clearIndex = arrayCenterPos - 10;
		if (clearIndex < 0)
			clearIndex += 21;
	}
	else
	{
		if (arrayCenterPos < 0)
			arrayCenterPos += 21;

		clearIndex = arrayCenterPos + 10;
		if (clearIndex > 20)
			clearIndex -= 21;
	}

	*titles[clearIndex] = 0;
	*subtitles[clearIndex] = 0;
	types[clearIndex] = 0;
	*dates[clearIndex] = 0;

}

uint8_t getEmptySpacesDown()
{
	uint8_t spaces = 0;
	for (int i = centerIndex; i <= centerIndex + 10; i++)
	{
		if (i >= numEntries)
			return 10;

		if (*getTitle(i) == 0)
		{
			break;
		}

		spaces++;
	}

	return spaces;
}

uint8_t getEmptySpacesUp()
{
	uint8_t spaces = 0;
	for (int i = centerIndex; i >= centerIndex - 10; i--)
	{
		if (i < 0)
			return 10;

		if (*getTitle(i) == 0)
		{
			break;
		}

		spaces++;
	}

	return spaces;
}

void requestNotification(uint16_t pos)
{
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 4);
	dict_write_uint16(iterator, 1, pos);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	ending = true;
}

void sendpickedEntry(int16_t pos, uint8_t mode)
{
	if (ending)
	{
		pickedEntry = pos;
		pickedMode = mode;

		setTitle(menu_layer_get_selected_index(listMenuLayer).row, "Ending!");
		menu_layer_reload_data(listMenuLayer);


		return;
	}

	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 5);
	dict_write_int16(iterator, 1, pos);
	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);

	setTitle(menu_layer_get_selected_index(listMenuLayer).row, "Loading!");
	menu_layer_reload_data(listMenuLayer);
}

void requestAdditionalEntries()
{
	if (ending)
		return;

	int emptyDown = getEmptySpacesDown();
	int emptyUp = getEmptySpacesUp();

	if (emptyDown < 6 && emptyDown <= emptyUp)
	{
		uint8_t startingIndex = centerIndex + emptyDown;
		requestNotification(startingIndex);
	}
	else if (emptyUp < 6)
	{
		uint8_t startingIndex = centerIndex - emptyUp;
		requestNotification(startingIndex);
	}
}

uint16_t menu_get_num_sections_callback(MenuLayer *me, void *data) {
	return 1;
}

uint16_t menu_get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data) {
	return numEntries;
}


int16_t menu_get_row_height_callback(MenuLayer *me,  MenuIndex *cell_index, void *data) {
	return 56;
}

void menu_pos_changed(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context)
{
	shiftArray(new_index.row);
	requestAdditionalEntries();
}


void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	graphics_context_set_text_color(ctx, GColorBlack);

	graphics_draw_text(ctx, getTitle(cell_index->row), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(34, 0, 144 - 35, 21), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, getSubtitle(cell_index->row), fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(34, 21, 144 - 35, 17), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
	graphics_draw_text(ctx, getDate(cell_index->row), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(34, 39, 144 - 35, 18), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

	GBitmap* image;
	switch (getType(cell_index->row))
	{
	case 1:
		image = ongoingNotification;
		break;
	default:
		image = normalNotification;
		break;
	}

	graphics_draw_bitmap_in_rect(ctx, image, GRect(1, 14, 31, 31));
}


void menu_select_callback(MenuLayer *me, MenuIndex *cell_index, void *data) {
	sendpickedEntry(cell_index->row, 0);
}

void receivedEntries(DictionaryIterator* data)
{
	uint16_t offset = dict_find(data, 1)->value->uint16;
	numEntries = dict_find(data, 2)->value->uint16;

	setType(offset, dict_find(data, 3)->value->uint8);
	setTitle(offset, dict_find(data, 4)->value->cstring);
	setSubtitle(offset, dict_find(data, 5)->value->cstring);
	setDate(offset, dict_find(data, 6)->value->cstring);

	menu_layer_reload_data(listMenuLayer);
	ending = false;

	if (pickedEntry >= 0)
	{
		sendpickedEntry(pickedEntry, pickedMode);
		return;
	}
	requestAdditionalEntries();
}

void list_data_received(int packetId, DictionaryIterator* data)
{
	switch (packetId)
	{
	case 2:
		receivedEntries(data);
		break;

	}

}

void list_window_load(Window *me) {
	normalNotification = gbitmap_create_with_resource(RESOURCE_ID_ICON);
	ongoingNotification = gbitmap_create_with_resource(RESOURCE_ID_COGWHEEL);

	Layer* topLayer = window_get_root_layer(listWindow);

	listMenuLayer = menu_layer_create(GRect(0, 0, 144, 168 - 16));

	// Set all the callbacks for the menu layer
	menu_layer_set_callbacks(listMenuLayer, NULL, (MenuLayerCallbacks){
		.get_num_sections = menu_get_num_sections_callback,
				.get_num_rows = menu_get_num_rows_callback,
				.get_cell_height = menu_get_row_height_callback,
				.draw_row = menu_draw_row_callback,
				.select_click = menu_select_callback,
				.selection_changed = menu_pos_changed
	});

	menu_layer_set_click_config_onto_window(listMenuLayer, listWindow);

	layer_add_child(topLayer, (Layer*) listMenuLayer);
}

void list_window_unload(Window *me) {
	gbitmap_destroy(normalNotification);
	gbitmap_destroy(ongoingNotification);

	menu_layer_destroy(listMenuLayer);

	window_destroy(me);
}

void list_window_appear(Window* me)
{
	setCurWindow(2);

	ending = false;
	pickedEntry = -1;
	pickedMode = 0;
}

void init_notification_list_window()
{
	listWindow = window_create();

	window_set_window_handlers(listWindow, (WindowHandlers){
		.appear = list_window_appear,
		.load = list_window_load,
		.unload = list_window_unload

	});

	window_set_fullscreen(listWindow, false);

	window_stack_push(listWindow, true /* Animated */);
}

