/*
 * NotificationsWindow.h
 *
 *  Created on: Aug 25, 2013
 *      Author: matej
 */

#ifndef NOTIFICATIONSWINDOW_H_
#define NOTIFICATIONSWINDOW_H_

void notification_received_data(uint8_t id, DictionaryIterator *received);
void notification_second_tick();
void notification_window_init();
void notification_data_sent(DictionaryIterator *received, void *context);

#endif /* NOTIFICATIONSWINDOW_H_ */
