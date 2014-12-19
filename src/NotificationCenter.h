/*
 * NotificationCenter.h
 *
 *  Created on: Aug 25, 2013
 *      Author: matej
 */

#ifndef NOTIFICATIONCENTER_H_
#define NOTIFICATIONCENTER_H_

extern uint8_t config_titleFont;
extern uint8_t config_subtitleFont;
extern uint8_t config_bodyFont;
extern uint16_t config_timeout;
extern uint16_t config_periodicTimeout;
extern uint8_t config_vibratePeriodically;
extern bool config_autoSwitchNotifications;
extern uint8_t config_vibrateMode;
extern bool config_dontClose;
extern bool config_showActive;
extern bool config_lightScreen;
extern bool config_dontVibrateWhenCharging;
extern bool config_invertColors;
extern bool config_disableNotifications;
extern bool config_disableVibration;
extern uint8_t config_shakeAction;
extern bool main_noMenu;

extern bool closingMode;
extern bool loadingMode;

void setCurWindow(uint8_t newWindow);
void switchWindow(uint8_t newWindow);
uint8_t getCurWindow(void);
const char* config_getFontResource(int id);
void closeApp(void);
bool canVibrate(void);
#endif /* NOTIFICATIONCENTER_H_ */
