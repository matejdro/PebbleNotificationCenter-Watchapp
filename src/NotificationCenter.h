/*
 * NotificationCenter.h
 *
 *  Created on: Aug 25, 2013
 *      Author: matej
 */

#ifndef NOTIFICATIONCENTER_H_
#define NOTIFICATIONCENTER_H_

#define STATUSBAR_Y_OFFSET STATUS_BAR_LAYER_HEIGHT

#ifdef PBL_PLATFORM_APLITE
    #define PBL_LOW_MEMORY
#endif

extern uint8_t config_titleFont;
extern uint8_t config_subtitleFont;
extern uint8_t config_bodyFont;
extern uint16_t config_timeout;
extern uint16_t config_periodicTimeout;
extern uint8_t config_vibratePeriodically;
extern bool config_autoSwitchNotifications;
extern uint8_t config_vibrateMode;
extern uint8_t config_lightTimeout;
extern uint8_t config_periodicVibrationPatternSize;
extern uint16_t config_periodicVibrationTotalDuration;
extern uint32_t* config_periodicVibrationPattern;

extern bool config_dontClose;
extern bool config_showActive;
extern bool config_lightScreen;
extern bool config_dontVibrateWhenCharging;
extern bool config_whiteText;
extern bool config_disableNotifications;
extern bool config_disableVibration;
extern bool config_displayScrollShadow;
extern bool config_scrollByPage;
extern bool config_disconnectedNotification;
extern bool config_gestures;

#ifdef PBL_COLOR
    extern bool config_skew_background_image_colors;
#endif

extern bool main_noMenu;

extern bool closingMode;
extern bool loadingMode;
extern bool rejectNotifications;

extern uint32_t appmessage_max_size;

void setCurWindow(uint8_t newWindow);
void switchWindow(uint8_t newWindow);
uint8_t getCurWindow(void);
const char* config_getFontResource(int id);
void closeApp(void);
bool canVibrate(void);
#endif /* NOTIFICATIONCENTER_H_ */
