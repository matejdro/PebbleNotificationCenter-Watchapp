/*
 * ActionMenu.h
 *
 *  Created on: Dec 19, 2014
 *      Author: matej
 */

#ifndef ACTIONMENU_H_
#define ACTIONMENU_H_

bool actions_menu_is_displayed(void);
int16_t actions_menu_get_selected_index(void);
char* actions_menu_get_action_text(int action);
void actions_menu_set_number_of_items(int items);
void actions_menu_reset_text(void);
void actions_menu_show(void);
void actions_menu_hide(void);
void actions_menu_move_up(void);
void actions_menu_move_down(void);
bool actions_menu_got_items(DictionaryIterator* dictionary);
void actions_menu_deinit(void);
void actions_menu_attach(Layer* layer);
void actions_menu_init(void);

#endif /* ACTIONMENU_H_ */
