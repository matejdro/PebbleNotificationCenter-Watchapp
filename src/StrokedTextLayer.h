//
// Created by Matej on 27.5.2015.
//

#ifndef DIALER2_STROKEDTEXTLAYER_H
#define DIALER2_STROKEDTEXTLAYER_H

#include <pebble.h>

typedef struct
{
    Layer* layer;
    char* text;
    GColor strokeColor;
    GColor textColor;
    GTextOverflowMode textOverflowMode;
    GTextAlignment textAlignment;
    GFont textFont;
    bool strokeEnabled;
} StrokedTextLayer;

StrokedTextLayer* stroked_text_layer_create(GRect frame);
void stroked_text_layer_destroy(StrokedTextLayer* strokedTextLayer);
void stroked_text_layer_set_text(StrokedTextLayer* strokedTextLayer, char* text);
void stroked_text_layer_set_text_color(StrokedTextLayer* strokedTextLayer, GColor textColor);
void stroked_text_layer_set_stroke_color(StrokedTextLayer *strokedTextLayer, GColor strokeColor);
void stroked_text_layer_set_font(StrokedTextLayer *strokedTextLayer, GFont font);
void stroked_text_layer_set_text_alignment(StrokedTextLayer *strokedTextLayer, GTextAlignment textAlignment);
void stroked_text_layer_set_text_overflow_mode(StrokedTextLayer *strokedTextLayer, GTextOverflowMode overflowMode);
Layer* stroked_text_layer_get_layer(StrokedTextLayer *strokedTextLayer);
GSize stroked_text_layer_get_content_size(StrokedTextLayer* strokedTextLayer);
void stroked_text_layer_set_stroke_enabled(StrokedTextLayer* strokedTextLayer, bool enabled);

#endif //DIALER2_STROKEDTEXTLAYER_H
