/**
 * Tertiary Text, originally by  vgmoose and PeterSumm
 * https://github.com/PeterSumm/tertiary_text
 */

#include "tertiary_text.h"
#include "ActionsMenu.h"
#include "Comm.h"

// Max text limit in characters
// You may adjust this to allow longer messages
#define MAX_CHARS 60

#define TOP 0
#define MID 1
#define BOT 2

static Window* window;

static TextLayer* text_title;
// static TextLayer* text_layer;
static TextLayer* text_input;

static TextLayer* buttons1[3];
static TextLayer* buttons2[3];
static TextLayer* buttons3[3];
static TextLayer** bbuttons[3];

static bool menu = false;

// Here are the three cases, or sets
static char caps[] =    	"ABCDEFGHIJKLM NOPQRSTUVWXYZ";
static char letters[] = 	"abcdefghijklm nopqrstuvwxyz";
static char numsym[] = 		"1234567890!?-'\"$()&*+#:@/,.";

// the below three strings just have to be unique, abc - xyz will be overwritten with the long strings above
static char* btext1[] = {"abc", "def", "ghi"};
static char* btext2[] = {"jkl", "m n", "opq"};
static char* btext3[] = {"rst", "uvw", "xyz"};
static char** btexts[] = {btext1, btext2, btext3};

// These are the actual sets that are displayed on each button, also need to be unique
static char set1[4] = "  a";
static char set2[4] = "  b";
static char set3[4] = "  c";
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

static char text_buffer[MAX_CHARS];
static int pos = 0;
static int top, end, size;

static const char* title;
static void* extra;
static TertiaryTextCallback callback;

static const bool animated = true;

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
        // Prevent overflowing
        if (pos == MAX_CHARS-1)
            pos--;

        // Type the character
        text_buffer[pos++] = master[top+b];
        drawNotepadText();
        change_set(cur_set, false);
        next();
    }

    drawSides();
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
    {
        actions_menu_move_up();
        return;
    }

    clickButton(TOP);
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void* context)
{
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

            if (sourceLength + pos < MAX_CHARS)
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

static void down_single_click_handler(ClickRecognizerRef recognizer, void* context)
{
    if (actions_menu_is_displayed())
    {
        actions_menu_move_down();
        return;
    }

    clickButton(BOT);
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

static void up_long_click_handler(ClickRecognizerRef recognizer, void* context)
{
    if (common_long(TOP)) return;

    set_menu();
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void* context)
{
    if (common_long(MID)) return;

    // Close this window

    actions_menu_show();
}


static void down_long_click_handler(ClickRecognizerRef recognizer, void* context) {

    if (common_long(BOT)) return;

    // delete or cancel when back is held
    if (size==27 && pos>0 )
    {
        text_buffer[--pos] = ' ';
        drawNotepadText();
    }
    else
    {
        next();
        drawSides();
    }

}

static void back_single_click_handler(ClickRecognizerRef recognizer, void* context) {

    if (actions_menu_is_displayed())
        actions_menu_hide();
    else
        window_stack_pop(true);
}

static void set_menu()
{
    menu = true;
    drawMenu();
}

static void click_config_provider(void* context)
{
    window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
    window_long_click_subscribe(BUTTON_ID_UP, 1000, up_long_click_handler, NULL);

    window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
    window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, NULL);

    window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
    window_long_click_subscribe(BUTTON_ID_DOWN, 1000, down_long_click_handler, NULL);

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

    }
    else if (size == 3)
    {
        for (int i=0; i<3; i++)
        {
            setlist[i][2] = master[top+i];
            text_layer_set_text(bbuttons[i][i==2], setlist[i]);
            text_layer_set_font(bbuttons[i][i==2], fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
        }
    }
}

static void clearText()
{
    for (int i=0; i<MAX_CHARS; i++)
        text_buffer[i] = '\0';
    pos = 0;
}

static void initSidesAndText()
{
    // Make sure the text is cleared on the first launch
    clearText();

    // Retrieve the window layer and its bounds
    Layer *window_layer = window_get_root_layer(window);
// 		GRect bounds = layer_get_bounds(window_layer);

    // Create a text layer for the text that is typed - WTF? - it was never used!
//     text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
//     text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
//     layer_add_child(window_layer, text_layer_get_layer(text_layer));

    // Create a text layer for the title
#if defined(PBL_ROUND)
    text_title = text_layer_create( GRect( 34, 16, 86, 32 ) );
#else
    text_title = text_layer_create( GRect( 3, 0, 107, 32 ) );
#endif
    text_layer_set_font( text_title, fonts_get_system_font( FONT_KEY_GOTHIC_14_BOLD ) );
    text_layer_set_text( text_title, title );
    layer_add_child( window_layer, text_layer_get_layer( text_title ) );

    // Create a text layer for the text that has been typed - PRS
#if defined(PBL_ROUND)
    text_input = text_layer_create((GRect) { .origin = { 32, 40 }, .size = { 90, 120 } });
    text_layer_set_font(text_input, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
#else
    text_input = text_layer_create((GRect) { .origin = { 3, 24 }, .size = { 110, 150 } });
    text_layer_set_font(text_input, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
#endif

    text_layer_set_background_color(text_input, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(text_input));

    for (int i = 0; i<3; i++)
    {
#if defined(PBL_ROUND)
        buttons1[i] = text_layer_create((GRect) { .origin = { 125, 12*i+14 }, .size = { 100, 100 } });
        buttons2[i] = text_layer_create((GRect) { .origin = { 125, 12*i+64 }, .size = { 100, 100 } });
        buttons3[i] = text_layer_create((GRect) { .origin = { 125, 12*i+114 }, .size = { 100, 100 } });
#else
        buttons1[i] = text_layer_create((GRect) { .origin = { 115, 12*i }, .size = { 100, 100 } });
        buttons2[i] = text_layer_create((GRect) { .origin = { 115, 12*i+50 }, .size = { 100, 100 } });
        buttons3[i] = text_layer_create((GRect) { .origin = { 115, 12*i+100 }, .size = { 100, 100 } });
#endif
    }

    for( int i=0; i<3; i++ )
        for( int j=0; j<3; j++ )
            layer_add_child( window_layer, text_layer_get_layer( bbuttons[i][j] ) );

    actions_menu_attach(window_layer);

}

static void drawNotepadText()
{
    text_layer_set_text(text_input, text_buffer);
}

static void window_unload(Window *window)
{
//   text_layer_destroy(text_layer);
    text_layer_destroy(text_input);
    text_layer_destroy(text_title);

    for( int i=0; i<3; i++ )
        for( int j=0; j<3; j++ )
            text_layer_destroy( bbuttons[ i ][ j ] );

    window_destroy(window);
    window = NULL;
}

static void window_load(Window* window)
{
    initSidesAndText();
    drawSides();
    drawNotepadText();
}

void tertiary_text_prompt( const char* _title, TertiaryTextCallback _callback, void* _extra )
{
    // Store the arguments for later
    title = _title;
    extra = _extra;
    callback = _callback;

    // Setup the button arrays
    bbuttons[0] = buttons1;
    bbuttons[1] = buttons2;
    bbuttons[2] = buttons3;

    // Create and configure the window
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);

    window_set_window_handlers(window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload,
    });

    // Default to lowercase letters
    change_set(1, true);

    // Initiate the character position and update the text layers
    next();

    // Push the window onto the stack
    window_stack_push(window, animated);
}

void tertiary_text_window_close()
{
    if (window != NULL)
        window_stack_remove(window, true);
}