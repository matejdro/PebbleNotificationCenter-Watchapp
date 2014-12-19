/*
 * NotificationsWindow.h
 *
 *  Created on: Aug 25, 2013
 *      Author: matej
 */

#ifndef NOTIFICATIONSWINDOW_H_
#define NOTIFICATIONSWINDOW_H_

void notification_window_received_data(uint8_t module, uint8_t id, DictionaryIterator *received);
void notification_window_init(void);
void notification_window_data_sent(void);
void notification_sendAction(int action);

#endif /* NOTIFICATIONSWINDOW_H_ */
