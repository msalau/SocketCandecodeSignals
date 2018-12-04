/**
 * @file main.c
 *
 * candecode App
 */

/**
Section: Included Files
*/

#include <stdio.h>
#include <stdlib.h>
#include "dbc.h"
#include "processFrame.h"

/**
Section: Definitions
*/

#define MAX_LINE_SIZE 100
#define MAX_DEVICE_NAME 100
#define MAX_ASC_FRAME 100

/**
Section: Implementation
*/

void printCallback(char *name, __u64 rawValue, const char *stringValue, double scaledValue, struct timeval tv, char *device)
{
	if (NULL == name)
	{
		printf("(%04ld.%06ld) %s: Frame 0x%02llx not found\n", tv.tv_sec, tv.tv_usec, device, rawValue);
	}
	else if (NULL != stringValue)
	{
		printf("(%04ld.%06ld) %s %s: 0x%02llx \"%s\"\n", tv.tv_sec, tv.tv_usec, device, name, rawValue, stringValue);
	}
	else
	{
		printf("(%04ld.%06ld) %s %s: 0x%02llx %f\n", tv.tv_sec, tv.tv_usec, device, name, rawValue, scaledValue);
	}
}

int main(int argc, char **argv)
{
	int process_all = 0;
	char buf[MAX_LINE_SIZE], device[MAX_DEVICE_NAME], ascframe[MAX_ASC_FRAME];

	char *frameName, *signalName;
	struct can_frame cf;
	struct timeval tv;

	Dbc_Frame_t *database = NULL;
	signal_callback_list_t *callbackList = NULL;
	Dbc_Signal_t *mySignal;
	Dbc_Frame_t *myFrame;

	if (argc < 2)
	{
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s Database [all]  # processes all frames\n", argv[0]);
		fprintf(stderr, "%s Database Message1.Signal1 [Message2.Signal2 Message3.Signal3]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Read DBC */
	if (Dbc_Init(&database, argv[1]))
	{
		fprintf(stderr, "[ERROR] Unable to open database %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	argc--;
	argv++;

	/* Decode all frames none were provided */
	if (argc == 1)
	{
		process_all = 1;
	}

	/* Parse arguments (frames/signals which should be decoded) */
	while (argc >= 2)
	{
		frameName = argv[1];
		if (strcmp(frameName, "all") == 0)
		{
			process_all = 1;
			break;
		}

		signalName = strchr(argv[1], '.');

		printf("Trying to find: Frame: %s", frameName);
		if (signalName != NULL)
		{
			*signalName = 0;
			signalName++;
			printf(", Signal: %s", signalName);
		}
		printf("\n");
		myFrame = Dbc_FindFrameByName(database, frameName);

		if (!myFrame)
		{
			fprintf(stderr, "[ERROR] Unable to find frame %s\n", frameName);
			exit(EXIT_FAILURE);
		}

		if (NULL != signalName)
		{
			mySignal = Dbc_FindSignalByName(myFrame, signalName);
			if (!mySignal)
			{
				fprintf(stderr, "[ERROR] Unable to find signal %s\n", signalName);
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			mySignal = NULL;
		}
		add_callback(&callbackList, myFrame, mySignal, printCallback, 0);

		printf("-- %s (0x%03x) ", myFrame->name, myFrame->canID);
		if (signalName != NULL)
			printf(" %s (%d [%d]) --", mySignal->name, mySignal->startBit, mySignal->signalLength);
		printf("\n");
		argc--;
		argv++;
	}

	while (fgets(buf, MAX_LINE_SIZE - 1, stdin))
	{

		if (sscanf(buf, "(%ld.%ld) %s %s", &tv.tv_sec, &tv.tv_usec, device, ascframe) != 4)
		{
			fprintf(stderr, "[ERROR] Incorrect line format in logfile\n");
			exit(EXIT_FAILURE);
		}
		if (parse_canframe(ascframe, &cf))
		{
			fprintf(stderr, "[ERROR] Unable to parse CAN frame from ASCII representation\n");
			exit(EXIT_FAILURE);
		}

		if (process_all)
			processAllFrames(database, printCallback, &cf, tv, device);
		else
			processFrame(callbackList, &cf, tv, device);
	}

	Dbc_DeInit(database);
	delete_callbacks(callbackList);
	return 0;
}
