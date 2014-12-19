#ifndef NOTIFYLIST_H_
#define NOTIFYLIST_H_

void list_window_init(void);
void list_window_data_received(int packetId, DictionaryIterator* data);
void list_window_data_sent(void);

#endif
