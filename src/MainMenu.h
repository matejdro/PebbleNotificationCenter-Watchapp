/*
 * MainMenu.h
 *
 *  Created on: Aug 25, 2013
 *      Author: matej
 */

#ifndef MAINMENU_H_
#define MAINMENU_H_

void init_menu_window();
void close_menu_window();
void show_menu();
void show_quit();
void show_old_watchapp();
void show_old_android();

extern char debugText[10];
void update_debug();

#endif /* MAINMENU_H_ */
