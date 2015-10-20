//
// Created by Matej on 20.10.2015.
//

#ifndef NOTIFICATIONCENTER_COMM_H
#define NOTIFICATIONCENTER_COMM_H

#include <pebble.h>

void nw_received_data_callback(uint8_t module, uint8_t id, DictionaryIterator* received);
void nw_data_sent_callback(void);
void nw_send_select_action(int32_t notificationId, uint8_t actionType);
void nw_send_list_notification_switch(int8_t change);
void nw_send_reply_text(char* text);
void nw_send_action_menu_result(int action);

#endif //NOTIFICATIONCENTER_COMM_H
