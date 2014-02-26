/*
 * MainMenu.h
 *
 *  Created on: Aug 25, 2013
 *      Author: matej
 */

#ifndef MAINMENU_H_
#define MAINMENU_H_

void init_menu_window();
void menu_data_received(int packetId, DictionaryIterator* data);
void close_menu_window();
void show_menu();
void show_old_watchapp();
void show_old_android();

#endif /* MAINMENU_H_ */
