//
// Created by Matej on 18.10.2015.
//
#ifdef PBL_COLOR
#include "BackgroundLighterLayer.h"
#include <pebble.h>
#include "../NotificationCenter.h"
#include "../util.h"

int getLuminance(GColor color)
{
    return 20 * color.r + 70 * color.g + 7 * color.b;
}

GColor getTextColor(GColor background)
{
    uint16_t luminance = getLuminance(background);
    return luminance > (MAX_LUMINANCE / 2) ? GColorBlack : GColorWhite;
}

void backgroud_lighter_layer_update(Layer* me, GContext* ctx)
{
    GBitmap* frameBuffer = graphics_capture_frame_buffer(ctx);

    GRect layerFrame = layer_get_frame(me);
    uint16_t x1 = layerFrame.origin.x;
    uint16_t y1 = layerFrame.origin.y;
    uint16_t x2 = x1 + layerFrame.size.w;
    uint16_t y2 = y1 + layerFrame.size.h;

    for (uint16_t y = y1; y < y2; y++)
    {
        GBitmapDataRowInfo info = gbitmap_get_data_row_info(frameBuffer, y);
        int startX = x1;
        int endX = x2 - 1;

        if (startX < info.min_x)
            startX = info.min_x;
        if (endX > info.max_x)
            endX = info.max_x;

        uint8_t* rowPixelData = info.data;

        for (uint16_t x = startX; x <= endX; x++)
        {
            GColor curPixel = (GColor8) {.argb = rowPixelData[x]};

            if (config_skew_background_image_colors)
            {
                if (config_whiteText)
                {
                    curPixel.r = max(0, curPixel.r - 3);
                    curPixel.g = max(0, curPixel.g - 2);
                    curPixel.b = max(0, curPixel.b - 1);

                }
                else
                {
                    curPixel.r = min(3, curPixel.r + 2);
                    curPixel.g = min(3, curPixel.g + 2);
                    curPixel.b = min(3, curPixel.b + 1);

                }
            }
            else
            {
                if (config_whiteText)
                {
                    curPixel.r = curPixel.r / 2;
                    curPixel.g = curPixel.g / 2;
                    curPixel.b = curPixel.b / 2;

                }
                else
                {
                    curPixel.r = min(3, curPixel.r + 2);
                    curPixel.g = min(3, curPixel.g + 2);
                    curPixel.b = min(3, curPixel.b + 2);
                }
            }

            rowPixelData[x] = curPixel.argb;
        }
    }

    graphics_release_frame_buffer(ctx, frameBuffer);
}
#endif