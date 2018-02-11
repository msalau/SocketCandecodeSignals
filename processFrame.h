
#if !defined _PROCESSFRAME_H_
#define _PROCESSFRAME_H_

#include "dbc.h"

struct signal_callback_list {
	Dbc_Frame_t *frame;
	Dbc_Signal_t *signal;
	__u64 rawValue;
	__u8 onChange; /* Callback every Signal/Message (0) or only on change of Signal (1) */
	void (*callback)(char *, __u64, double, struct timeval, char *device);
	UT_hash_handle hh;
};

void add_callback(struct signal_callback_list **callbackList, 
			Dbc_Frame_t *frame, Dbc_Signal_t *signal,
			void  (*callback)(char *, __u64, double, struct timeval, char *device), __u8 onChange);
			
void processFrame(struct signal_callback_list *callbackList, struct can_frame *cf, struct timeval tv, char *device);

#endif
