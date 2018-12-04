/**
 * @file processFrame.h
 *
 * Process Messages and Signals
 */

#ifndef _PROCESSFRAME_H_
#define _PROCESSFRAME_H_

#include "dbc.h"

typedef void (*callback_t)(const char *, const char *, __u64, const char *, double, struct timeval, char *device);

typedef struct
{
	Dbc_Frame_t *frame;
	Dbc_Signal_t *signal;
	__u64 rawValue;
	__u8 onChange;  /* Callback every Signal/Message (0) or only on change of Signal (1) */
	callback_t callback;

	UT_hash_handle hh;
} signal_callback_list_t;

void add_callback(signal_callback_list_t **callbackList, Dbc_Frame_t *frame, Dbc_Signal_t *signal, callback_t callback, __u8 onChange);
void delete_callbacks(signal_callback_list_t *callbackList);
void processAllFrames(Dbc_Frame_t *frames, callback_t callback, struct can_frame *cf, struct timeval tv, char *device);
void processFrame(signal_callback_list_t *callbackList, struct can_frame *cf, struct timeval tv, char *device);

#endif
