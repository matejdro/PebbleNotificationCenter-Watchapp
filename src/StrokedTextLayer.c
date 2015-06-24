#include <pebble.h>
#include "StrokedTextLayer.h"

static void stroked_text_layer_paint(Layer *layer, GContext *ctx);

StrokedTextLayer* stroked_text_layer_create(GRect frame)
{
    StrokedTextLayer* strokedTextLayer = malloc(sizeof(StrokedTextLayer));

    strokedTextLayer->layer = layer_create_with_data(frame, sizeof(StrokedTextLayer*));
    *((StrokedTextLayer**) layer_get_data(strokedTextLayer->layer)) = strokedTextLayer;
    layer_set_update_proc(strokedTextLayer->layer, stroked_text_layer_paint);

    strokedTextLayer->text = NULL;
    strokedTextLayer->strokeColor = GColorWhite;
    strokedTextLayer->textColor = GColorBlack;
    strokedTextLayer->textFont = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    strokedTextLayer->textAlignment = GTextAlignmentCenter;
    strokedTextLayer->textOverflowMode = GTextOverflowModeWordWrap;
    strokedTextLayer->strokeEnabled = true;

    return strokedTextLayer;
}

void stroked_text_layer_destroy(StrokedTextLayer* strokedTextLayer)
{
    layer_destroy(strokedTextLayer->layer);
    free(strokedTextLayer);
}

void stroked_text_layer_set_text(StrokedTextLayer* strokedTextLayer, char* text)
{
    strokedTextLayer->text = text;
    layer_mark_dirty(strokedTextLayer->layer);
}


void stroked_text_layer_set_text_color(StrokedTextLayer *strokedTextLayer, GColor textColor)
{
    strokedTextLayer->textColor = textColor;
    layer_mark_dirty(strokedTextLayer->layer);
}

void stroked_text_layer_set_stroke_color(StrokedTextLayer *strokedTextLayer, GColor strokeColor)
{
    strokedTextLayer->strokeColor = strokeColor;
    layer_mark_dirty(strokedTextLayer->layer);
}


void stroked_text_layer_set_font(StrokedTextLayer* strokedTextLayer, GFont font)
{
    strokedTextLayer->textFont = font;
    layer_mark_dirty(strokedTextLayer->layer);
}


void stroked_text_layer_set_text_alignment(StrokedTextLayer* strokedTextLayer, GTextAlignment textAlignment)
{
    strokedTextLayer->textAlignment = textAlignment;
    layer_mark_dirty(strokedTextLayer->layer);
}

void stroked_text_layer_set_text_overflow_mode(StrokedTextLayer* strokedTextLayer, GTextOverflowMode overflowMode)
{
    strokedTextLayer->textOverflowMode = overflowMode;
    layer_mark_dirty(strokedTextLayer->layer);
}

Layer* stroked_text_layer_get_layer(StrokedTextLayer* strokedTextLayer)
{
    return strokedTextLayer->layer;
}


GSize stroked_text_layer_get_content_size(StrokedTextLayer* strokedTextLayer)
{
    GRect reducedFrame = layer_get_bounds(strokedTextLayer->layer);
    reducedFrame.size.w -= 2;
    reducedFrame.size.h -= 2;


    GSize result = graphics_text_layout_get_content_size(strokedTextLayer->text, strokedTextLayer->textFont, reducedFrame, strokedTextLayer->textOverflowMode, strokedTextLayer->textAlignment);
    result.h += 2;
    result.w += 2;

    return result;
}


void stroked_text_layer_set_stroke_enabled(StrokedTextLayer* strokedTextLayer, bool enabled)
{
    strokedTextLayer->strokeEnabled = enabled;
    layer_mark_dirty(strokedTextLayer->layer);
}

static void stroked_text_layer_paint(Layer* layer, GContext *ctx)
{
    StrokedTextLayer* strokedTextLayer = *((StrokedTextLayer**) layer_get_data(layer));

    GRect bounds = layer_get_bounds(layer);
    GPoint origin = bounds.origin;
    GSize size = bounds.size;
    size.h -= 2;
    size.w -= 2;


    if (strokedTextLayer->strokeEnabled)
    {
        graphics_context_set_text_color(ctx, strokedTextLayer->strokeColor);
        graphics_draw_text(ctx, strokedTextLayer->text, strokedTextLayer->textFont, GRect(origin.x, origin.y + 1, size.w, size.h), strokedTextLayer->textOverflowMode, strokedTextLayer->textAlignment, NULL);
        graphics_draw_text(ctx, strokedTextLayer->text, strokedTextLayer->textFont, GRect(origin.x + 1, origin.y, size.w, size.h), strokedTextLayer->textOverflowMode, strokedTextLayer->textAlignment, NULL);
        graphics_draw_text(ctx, strokedTextLayer->text, strokedTextLayer->textFont, GRect(origin.x + 2, origin.y + 1, size.w, size.h), strokedTextLayer->textOverflowMode, strokedTextLayer->textAlignment, NULL);
        graphics_draw_text(ctx, strokedTextLayer->text, strokedTextLayer->textFont, GRect(origin.x + 1, origin.y + 2, size.w, size.h), strokedTextLayer->textOverflowMode, strokedTextLayer->textAlignment, NULL);
    }

    graphics_context_set_text_color(ctx, strokedTextLayer->textColor);
    graphics_draw_text(ctx, strokedTextLayer->text, strokedTextLayer->textFont, GRect(origin.x + 1, origin.y + 1, size.w, size.h), strokedTextLayer->textOverflowMode, strokedTextLayer->textAlignment, NULL);
}

