/**
 * @file processFrame.c
 *
 * Process Messages and Signals
 */

/**
Section: Included Files
*/

#include <stdio.h>
#include <stdlib.h>
#include "processFrame.h"
#include "stdbool.h"
#include "libcan-encode-decode/include/can_encode_decode_inl.h"

float toPhysicalValue(uint64_t target, float factor, float offset, bool is_signed);
uint64_t extractSignal(const uint8_t* frame, const uint8_t startbit, const uint8_t length, bool is_big_endian, bool is_signed);

void add_callback(signal_callback_list_t **callbackList, Dbc_Frame_t *frame, Dbc_Signal_t *signal, callback_t callback, __u8 onChange)
{
	signal_callback_list_t *callbackItem;

	callbackItem = malloc(sizeof(signal_callback_list_t));
	callbackItem->frame = frame;
	callbackItem->signal = signal;
	callbackItem->callback = callback;
	callbackItem->rawValue = 0;
	callbackItem->onChange = onChange;

	HASH_ADD_INT(*callbackList, signal, callbackItem);
}

void delete_callbacks(signal_callback_list_t *callbackList)
{
	signal_callback_list_t *callback, *callback_tmp;

	HASH_ITER(hh, callbackList, callback, callback_tmp)
	{
		HASH_DEL(callbackList, callback);
		free(callback);
	}
}

void processAllFrames(Dbc_Frame_t *frames, callback_t callback, struct can_frame *cf, struct timeval tv, char *device)
{
	Dbc_Frame_t *frame;
	Dbc_Signal_t *signal;
	__u64 value = 0;
	double scaled = 0.;
	unsigned int muxerVal = 0;
	const char *stringVal = NULL;
	int frame_found = 0;

	for (frame = frames; frame != NULL; frame = frame->hh.next)
	{
		/* Matching CAN frame */
		if (frame->canID == cf->can_id)
		{
			frame_found = 1;

			if (frame->isMultiplexed)
			{
				/* Find multiplexer */
				for (signal = frame->signals; signal != NULL; signal = signal->hh.next)
				{
					if (DBC_MUX_DEFINE == signal->isMultiplexer)
					{
						muxerVal = extractSignal(cf->data, signal->startBit, signal->signalLength, (bool) signal->is_big_endian, signal->is_signed);
						scaled = toPhysicalValue(muxerVal, signal->factor, signal->offset, signal->is_signed);
						stringVal = Dbc_FindValueString(signal, muxerVal);
						callback(signal->name, muxerVal, stringVal, scaled, tv, device);
						break;
					}
				}

				for (signal = frame->signals; signal != NULL; signal = signal->hh.next)
				{
					/* decode not multiplexed signals and signals with correct muxVal */
					if (DBC_MUX_NONE == signal->isMultiplexer || (DBC_MUX_DATA == signal->isMultiplexer && signal->muxId == muxerVal))
					{
						value = extractSignal(cf->data, signal->startBit, signal->signalLength, (bool) signal->is_big_endian, signal->is_signed);
						scaled = toPhysicalValue(value, signal->factor, signal->offset, signal->is_signed);
						stringVal = Dbc_FindValueString(signal, value);
						callback(signal->name, value, stringVal, scaled, tv, device);
					}
				}
			}
			else
			{
				for (signal = frame->signals; signal != NULL; signal = signal->hh.next)
				{
					value = extractSignal(cf->data, signal->startBit, signal->signalLength, (bool) signal->is_big_endian, signal->is_signed);
					scaled = toPhysicalValue(value, signal->factor, signal->offset, signal->is_signed);
					stringVal = Dbc_FindValueString(signal, value);
					callback(signal->name, value, stringVal, scaled, tv, device);
				}
			}
		}
	}

	if (!frame_found)
	{
		value = cf->can_id;  /* place frame ID in actual value (dirty hack) */
		callback(NULL, value, NULL, scaled, tv, device);
	}
}

void processFrame(signal_callback_list_t *callbackList, struct can_frame *cf, struct timeval tv, char *device)
{
	signal_callback_list_t *callbackItem;
	Dbc_Signal_t *signal;
	__u64 value = 0;
	double scaled = 0.;
	const char *stringVal = NULL;
	unsigned int muxerVal = 0;

	/* Iterate through all callback elements */
	for (callbackItem = callbackList; callbackItem != NULL; callbackItem = callbackItem->hh.next)
	{
		/* Matching CAN frame */
		if (callbackItem->frame->canID == cf->can_id)
		{
			if (callbackItem->frame->isMultiplexed)
			{
				/* Find multiplexer */
				for (signal = callbackItem->frame->signals; signal != NULL; signal = signal->hh.next)
				{
					if (DBC_MUX_DEFINE == signal->isMultiplexer)
					{
						muxerVal = extractSignal(cf->data, signal->startBit, signal->signalLength, (bool) signal->is_big_endian, signal->is_signed);
						scaled = toPhysicalValue(muxerVal, signal->factor, signal->offset, signal->is_signed);
						stringVal = Dbc_FindValueString(signal, muxerVal);
						(callbackItem->callback)(signal->name, muxerVal, stringVal, scaled, tv, device);
						break;
					}
				}
			}

			if (callbackItem->signal == NULL)
			{
				/* Process all signals in message */

				if (callbackItem->frame->isMultiplexed)
				{
					for (signal = callbackItem->frame->signals; signal != NULL; signal = signal->hh.next)
					{
						/* decode not multiplexed signals and signals with correct muxVal */
						if (DBC_MUX_NONE == signal->isMultiplexer || (DBC_MUX_DATA == signal->isMultiplexer && signal->muxId == muxerVal))
						{
							value = extractSignal(cf->data, signal->startBit, signal->signalLength, (bool) signal->is_big_endian, signal->is_signed);
							scaled = toPhysicalValue(value, signal->factor, signal->offset, signal->is_signed);
							stringVal = Dbc_FindValueString(signal, value);
							(callbackItem->callback)(signal->name, value, stringVal, scaled, tv, device);
						}
					}

				}
				else
				{
					for (signal = callbackItem->frame->signals; signal != NULL; signal = signal->hh.next)
					{
						value = extractSignal(cf->data, signal->startBit, signal->signalLength, (bool) signal->is_big_endian, signal->is_signed);
						scaled = toPhysicalValue(value, signal->factor, signal->offset, signal->is_signed);
						stringVal = Dbc_FindValueString(signal, value);
						(callbackItem->callback)(signal->name, value, stringVal, scaled, tv, device);
					}
				}
			}
			else
			{
				if (callbackItem->frame->isMultiplexed)
				{
					if (DBC_MUX_NONE == callbackItem->signal->isMultiplexer || (DBC_MUX_DATA == callbackItem->signal->isMultiplexer && callbackItem->signal->muxId == muxerVal))
					{
						value = extractSignal(cf->data, callbackItem->signal->startBit, callbackItem->signal->signalLength, (bool) callbackItem->signal->is_big_endian,
								callbackItem->signal->is_signed);
						scaled = toPhysicalValue(value, callbackItem->signal->factor, callbackItem->signal->offset, callbackItem->signal->is_signed);
						stringVal = Dbc_FindValueString(callbackItem->signal, value);
						callbackItem->rawValue = value;
						(callbackItem->callback)(callbackItem->signal->name, value, stringVal, scaled, tv, device);
					}
				}
				else
				{
					if ((0 == callbackItem->onChange) || (callbackItem->rawValue != value))
					{
						value = extractSignal(cf->data, callbackItem->signal->startBit, callbackItem->signal->signalLength, (bool) callbackItem->signal->is_big_endian,
								callbackItem->signal->is_signed);
						scaled = toPhysicalValue(value, callbackItem->signal->factor, callbackItem->signal->offset, callbackItem->signal->is_signed);
						stringVal = Dbc_FindValueString(callbackItem->signal, value);
						callbackItem->rawValue = value;
						(callbackItem->callback)(callbackItem->signal->name, value, stringVal, scaled, tv, device);
					}
				}
			}
		}
	}
}
