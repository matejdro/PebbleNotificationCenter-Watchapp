/*
 * ActionMenu.c
 *
 *  Created on: Dec 19, 2014
 *      Author: matej
 */

#include <pebble.h>
#include "tertiary_text.h"

static Layer* menuBackground;
static MenuLayer* menu;
static bool actionsMenuDisplayed = false;
static char actions[20][20];
static uint8_t numOfActions = 0;

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
	return numOfActions;
}


static int16_t menu_get_row_height_callback(MenuLayer *me,  MenuIndex *cell_index, void *data) {
	return 30;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorBlack, menu_cell_layer_is_highlighted(cell_layer) ? GColorWhite : GColorBlack));
	graphics_draw_text(ctx, actions[cell_index->row], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(5, 0, 144 - 20 - 10, 27), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}


void actions_menu_reset_text(void)
{
	for (int i = 0; i < 20; i++)
	{
		actions[i][0] = 0;
	}
}

bool actions_menu_is_displayed(void)
{
	return actionsMenuDisplayed;
}

int16_t actions_menu_get_selected_index(void)
{
	return menu_layer_get_selected_index(menu).row;
}

char* actions_menu_get_action_text(int action)
{
	return actions[action];
}

void actions_menu_set_number_of_items(int items)
{
	numOfActions = items;
}

void actions_menu_move_up(void)
{
	MenuIndex index = menu_layer_get_selected_index(menu);
	if (index.row == 0)
	{
		menu_layer_set_selected_index(menu, MenuIndex(0,numOfActions - 1), MenuRowAlignCenter, true);
		return;
	}

	menu_layer_set_selected_next(menu, true, MenuRowAlignCenter, true);
}

void actions_menu_move_down(void)
{
	MenuIndex index = menu_layer_get_selected_index(menu);
	if (index.row == numOfActions - 1)
	{
		menu_layer_set_selected_index(menu, MenuIndex(0, 0), MenuRowAlignCenter, true);
		return;
	}

	menu_layer_set_selected_next(menu, false, MenuRowAlignCenter, true);
}

void actions_menu_show(void)
{
	menu_layer_reload_data(menu);
	MenuIndex firstItem;
	firstItem.row = 0;
	firstItem.section = 0;
	menu_layer_set_selected_index(menu, firstItem, MenuRowAlignTop, false);
	actionsMenuDisplayed = true;
	layer_set_hidden((Layer*) menuBackground, false);
}

void actions_menu_hide(void)
{
	actionsMenuDisplayed = false;
	layer_set_hidden((Layer*) menuBackground, true);
}

bool actions_menu_got_items(DictionaryIterator* dictionary)
{
	bool menuPoppedUp = false;

	uint8_t* data = dict_find(dictionary, 2)->value->data;
	uint8_t* text = dict_find(dictionary, 3)->value->data;

	uint8_t firstId = data[0];
	numOfActions = data[1];
	bool displayTertiaryText = data[2] != 0;

	uint8_t packetSize = numOfActions - firstId;

	if (firstId == 0)
	{
		if (displayTertiaryText)
		{
			actions_menu_hide();
			tertiary_text_prompt("Enter reply:", NULL, NULL);
		}
		else if (!actions_menu_is_displayed())
		{
			actions_menu_reset_text();
			actions_menu_show();
		}

		menuPoppedUp = true;
	}

	if (packetSize > 4)
		packetSize = 4;

	for (int i = 0; i < packetSize; i++)
	{
		memcpy(actions[i + firstId], &text[i*19],  19);
	}

	if (actionsMenuDisplayed)
		menu_layer_reload_data(menu);

	return menuPoppedUp;
}

void actions_menu_init(void)
{
	const int screenXOffset = PBL_IF_ROUND_ELSE(27, 9);

	menuBackground = layer_create(GRect(screenXOffset, 9 + 16, 144 - 18, 168 - 34));
    layer_set_update_proc(menuBackground, menu_paint_background);
    layer_set_hidden((Layer*) menuBackground, true);

	menu = menu_layer_create(GRect(1, 1, 144 - 20, 168 - 36));

	menu_layer_set_callbacks(menu, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_cell_height = menu_get_row_height_callback,
		.draw_row = menu_draw_row_callback,
	});

	layer_add_child(menuBackground, menu_layer_get_layer(menu));

#ifdef PBL_COLOR
	menu_layer_set_highlight_colors(menu, GColorChromeYellow, GColorBlack);
#endif

menu_layer_set_center_focused(menu, false);
}

void actions_menu_attach(Layer* layer)
{
	layer_add_child(layer, menuBackground);
}

void actions_menu_deinit(void)
{
	layer_destroy(menuBackground);
	menu_layer_destroy(menu);
}
