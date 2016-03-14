//
// Created by Matej on 18.10.2015.
//
#ifdef PBL_COLOR
#include "BackgroundLighterLayer.h"
#include <pebble.h>
#include "../NotificationCenter.h"

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

            if (config_whiteText)
            {
                //Decrease luminance of the pixel until it is dark enough to not make text in front unreadable
                while (getLuminance(curPixel) > MAX_LUMINANCE * 2 / 5)
                {
                    if (curPixel.r > 0)
                        curPixel.r--;
                    if (curPixel.g > 0)
                        curPixel.g--;
                    if (curPixel.b > 0)
                        curPixel.b--;
                }
            }
            else
            {
                //Increase luminance of the pixel until it is bright enough to not make text in front unreadable
                while (getLuminance(curPixel) < MAX_LUMINANCE * 2 / 5)
                {
                    if (curPixel.r < 3)
                        curPixel.r++;
                    if (curPixel.g < 3)
                        curPixel.g++;
                    if (curPixel.b < 3)
                        curPixel.b++;
                }
            }

            rowPixelData[x] = curPixel.argb;
        }
    }

    graphics_release_frame_buffer(ctx, frameBuffer);
}
#endif