//
// Created by Matej on 18.10.2015.
//
#ifdef PBL_COLOR
    #ifndef NOTIFICATIONCENTER_BACKGROUNDLIGHTERLAYER_H
    #define NOTIFICATIONCENTER_BACKGROUNDLIGHTERLAYER_H

    #include <pebble.h>

    #define MAX_LUMINANCE 291

    /**
     * Returns Luminance of the color ranging between 0 and MAX_LUMINANCE.
     */
    int getLuminance(GColor color);
    GColor getTextColor(GColor background);
    void backgroud_lighter_layer_update(Layer* me, GContext* ctx);

    #endif //NOTIFICATIONCENTER_BACKGROUNDLIGHTERLAYER_H
#endif