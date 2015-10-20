#include <pebble.h>
#include "ActionsMenu.h"
#include "../NotificationCenter.h"
#include "Comm.h"

#define TOP 0
#define MID 1
#define BOT 2

#define NOTEPAD_TEXT 1
#define NOTEPAD_CHAR_COUNT 2

#define TEXT_LIMIT 60

static Window* window = NULL;

//static TextLayer* text_layer;
static TextLayer* wordsYouWrite;

static TextLayer* buttons1[3];
static TextLayer* buttons2[3];
static TextLayer* buttons3[3];
static TextLayer** bbuttons[3];

static Layer* actionsProxyLayer;

#ifdef PBL_SDK_3
    static StatusBarLayer* statusBar;
#endif

static bool menu = false;

// Here are the three cases, or sets
static char caps[] =    "ABCDEFGHIJKLM NOPQRSTUVWXYZ";
static char letters[] = "abcdefghijklm nopqrstuvwxyz";
static char numsym[] = "1234567890!?-'\"$()&*+#:@/,.";

// the below three strings just have to be unique, abc - xyz will be overwritten with the long strings above
static char* btext1[] = {"abc\0", "def\0", "ghi\0"};
static char* btext2[] = {"jkl\0", "m n\0", "opq\0"};
static char* btext3[] = {"rst\0", "uvw\0", "xyz\0"};
static char** btexts[] = {btext1, btext2, btext3};

// These are the actual sets that are displayed on each button, also need to be unique
static char set1[4] = "  a\0";
static char set2[4] = "  b\0";
static char set3[4] = "  c\0";
static char* setlist[] = {set1, set2, set3};

static char* cases[] = {"CAP", "low", "#@1"};

static int cur_set = 1;

static void drawSides();
static void drawMenu();
static void set_menu();
static void drawNotepadText();

static char* rotate_text[] = {caps, letters, numsym};
static void next();

static char* master = letters;


static char text_buffer[TEXT_LIMIT + 1];
static int pos = 0;
static int top, end, size;

// This function changes the next case/symbol set.
static void change_set(int s, bool lock)
{
    int count = 0;
    master = rotate_text[s];
    for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++)
        {
            for (int k=0; k<3; k++)
            {
                btexts[i][j][k] = master[count];
                count++;
            }
        }
    }
    
    menu = false;
    if (lock) cur_set = s;
    
    drawSides();
}

static void next()
{
    top = 0;
    end = 26;
    size = 27;
}

static void clickButton(int b)
{
	if (menu)
	{
		change_set(b, false);
		return;
	}

	if (size > 3)
	{
		size /= 3;
		if (b == TOP)
		end -= 2*size;
		else if (b == MID)
		{
			top += size;
			end -= size;
		}
		else if (b == BOT)
		top += 2*size;
	}
	else
	{
		if (pos < TEXT_LIMIT)
		{
			text_buffer[pos++] = master[top+b];
			drawNotepadText();
		}

		change_set(cur_set, false);
		next();
	}

	drawSides();
    
}

// Modify these common button handlers
static void up_single_click_handler(ClickRecognizerRef recognizer, void* context) {
    if (actions_menu_is_displayed())
    {
		actions_menu_move_up();
    	return;
    }

    clickButton(TOP);
    
}

static void up_repeating_click_handler(ClickRecognizerRef recognizer, void* context) {

	if (actions_menu_is_displayed())
		actions_menu_move_up();
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void* context) {
	if (actions_menu_is_displayed())
	{
		int16_t pickedItem = actions_menu_get_selected_index();
		if (pickedItem == 0)
		{
            nw_send_reply_text(text_buffer);
			actions_menu_hide();
			window_stack_pop(true);
		}
		else
		{
			char* copySource = actions_menu_get_action_text(pickedItem);
			int sourceLength = strlen(copySource);

			if (sourceLength + pos < TEXT_LIMIT)
			{
				strcpy(&text_buffer[pos], copySource);
				pos += sourceLength;
				drawNotepadText();
				actions_menu_hide();
			}
		}

		return;
	}

    clickButton(MID);
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void* context) {
    
    if (actions_menu_is_displayed())
    {
		actions_menu_move_down();
    	return;
    }

    clickButton(BOT);
}

static void down_repeating_click_handler(ClickRecognizerRef recognizer, void* context) {

	if (actions_menu_is_displayed())
		actions_menu_move_down();
}


static void back_single_click_handler(ClickRecognizerRef recognizer, void* context) {

	if (actions_menu_is_displayed())
		actions_menu_hide();
	else
		window_stack_pop(true);
}

static bool common_long(int b)
{
    if (menu)
    {
        change_set(b, true);
        return true;
    }
    return false;
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void* context) {
    
    if (common_long(TOP)) return;
    
    set_menu();
    
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void* context) {
    
    if (common_long(MID)) return;

    actions_menu_set_number_of_items(10);
    actions_menu_show();

}


static void down_long_click_handler(ClickRecognizerRef recognizer, void* context) {
    
    if (common_long(BOT)) return;
    
    // delete or cancel when back is held
    
    if (size==27 && pos>0)
    {
        text_buffer[--pos] = 0;
        drawNotepadText();
    }
    else
    {
        next();
        drawSides();
    }
    
}

static void set_menu()
{
	menu = true;
	drawMenu();
}

// This usually won't need to be modified

static void click_config_provider(void* context) {
    
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
    window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
	//window_single_repeating_click_subscribe(BUTTON_ID_UP, 50, up_repeating_click_handler);

    window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
    window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
    
    window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
    window_long_click_subscribe(BUTTON_ID_DOWN, 500, down_long_click_handler, NULL);
	//window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 50, down_repeating_click_handler);

    window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler);

}

static void drawMenu()
{
    for (int i=0; i<3; i++)
    {
        text_layer_set_text(bbuttons[i][i!=2], " ");
        text_layer_set_text(bbuttons[i][2], " ");
        
        text_layer_set_text(bbuttons[i][i==2], cases[i]);
        text_layer_set_font(bbuttons[i][0], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    }
}


// This method draws the characters on the right side near the buttons
static void drawSides()
{
    if (size==27) // first click (full size)
    {
        // update all 9 labels to their proper values
        for (int h=0; h<3; h++)
        {
            for (int i=0; i<3; i++)
            {
                text_layer_set_text(bbuttons[h][i], btexts[h][i]);
                text_layer_set_background_color(bbuttons[h][i], GColorClear);
                text_layer_set_font(bbuttons[h][i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
            }
            
        }
    }
    else if (size==9)   // second click
    {
        
        for (int i=0; i<3; i++)
        {
            text_layer_set_text(bbuttons[i][i!=2], " ");
            text_layer_set_text(bbuttons[i][2], " ");
            
            text_layer_set_text(bbuttons[i][i==2], btexts[top/9][i]);
            text_layer_set_font(bbuttons[i][i==2], fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        }
        
    } else if (size == 3)
    {
        for (int i=0; i<3; i++)
        {
            setlist[i][2] = master[top+i];
            text_layer_set_text(bbuttons[i][i==2], setlist[i]);
            
        }
    }
    
}

static void initSidesAndText()
{
    Layer *window_layer = window_get_root_layer(window);

    wordsYouWrite = text_layer_create((GRect) { .origin = { 10, STATUSBAR_Y_OFFSET }, .size = { 100, 150 } });

    text_layer_set_background_color(wordsYouWrite, GColorClear);
    text_layer_set_font(wordsYouWrite, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(wordsYouWrite));
    
    for (int i = 0; i<3; i++)
    {
        buttons1[i] = text_layer_create((GRect) { .origin = { 115, 12*i + STATUSBAR_Y_OFFSET }, .size = { 100, 100 } });
        buttons2[i] = text_layer_create((GRect) { .origin = { 115, 12*i+50 + STATUSBAR_Y_OFFSET }, .size = { 100, 100 } });
        buttons3[i] = text_layer_create((GRect) { .origin = { 115, 12*i+100 + STATUSBAR_Y_OFFSET }, .size = { 100, 100 } });
    }
    
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            layer_add_child(window_layer, text_layer_get_layer(bbuttons[i][j]));
    
    actionsProxyLayer = layer_create(GRect(0, -16 + STATUSBAR_Y_OFFSET, 144, 168));
    actions_menu_attach(actionsProxyLayer);
    layer_add_child(window_layer, actionsProxyLayer);
}

static void drawNotepadText()
{
    text_layer_set_text(wordsYouWrite, text_buffer);
}

static void deinit(void) {
    text_buffer[pos] = '\0';
    
    text_layer_destroy(wordsYouWrite);
    for (int i = 0; i<3; i++)
	{
        text_layer_destroy(buttons1[i]);
        text_layer_destroy(buttons2[i]);
        text_layer_destroy(buttons3[i]);
	}

#ifdef PBL_SDK_3
    status_bar_layer_destroy(statusBar);
#endif

    window_destroy(window);
}

static void window_unload(Window *window) {
    deinit();
}

static void window_load(Window* window)
{
    Layer *topLayer = window_get_root_layer(window);

#ifdef PBL_SDK_3
    statusBar = status_bar_layer_create();
    layer_add_child(topLayer, status_bar_layer_get_layer(statusBar));
#endif

    initSidesAndText();
    drawSides();

}

static void init(void) {
	//List window needs to close, otherwise Pebble runs out of RAM.
	//close_notification_list_window();

    window = window_create();
    
    text_buffer[0] = 0;
    pos = 0;
    
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    
    const bool animated = true;
    window_stack_push(window, animated);

}

void tertiary_text_window_close()
{
	if (window != NULL)
		window_stack_remove(window, true);
}

void tertiary_text_window_init() {
    bbuttons[0] = buttons1;
    bbuttons[1] = buttons2;
    bbuttons[2] = buttons3;
    init();
    change_set(1, true);
    next();
    drawSides();
    drawNotepadText();
    
    //deinit();
}
