//
// Created by Matej on 20.10.2015.
//

#ifndef NOTIFICATIONCENTER_UI_H
#define NOTIFICATIONCENTER_UI_H

void nw_ui_refresh_notification(void);
void nw_ui_refresh_picked_indicator(void);

void nw_ui_scroll_to_notification_start(void);
void nw_ui_set_busy_indicator(bool value);
void nw_ui_scroll_notification(bool down);
void nw_ui_update_statusbar_clock();

void nw_ui_load(Window* window);
void nw_ui_unload();

#ifdef PBL_COLOR
    void nw_ui_set_notification_image(GBitmap* image);
#endif

#endif //NOTIFICATIONCENTER_UI_H
