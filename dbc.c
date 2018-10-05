/**
 * @file dbc.c
 *
 * DBC handler
 */

/**
Section: Included Files
*/

#include <stdio.h>
#include <stdint.h>

#include "dbc.h"

/**
Section: Private functions
*/

/**
 * @brief      If Big Endian this function will recompute the start bit
 * 
 * Code is from:
 * https://github.com/julietkilo/CANBabel/blob/master/src/main/java/com/github/canbabel/canio/dbc/DbcReader.java
 *
 * @param[in]  byteOrder     The byte order
 * @param[in]  startBit      The start bit
 * @param[in]  signalLength  The signal length
 *
 * @return     New start bit (if Big Endian); unchanged start bit otherwise
 */
static int Dbc_ProcessStartBit(int byteOrder, int startBit, int signalLength);

/**
Section: Implementation
*/

void Dbc_AddFrame(Dbc_Frame_t **db, int32_t canID, uint8_t dlc, char *frameName)
{
    Dbc_Frame_t *s;

    s = malloc(sizeof(Dbc_Frame_t));
    s->canID = canID;
    s->dlc = dlc;
    s->signals = NULL;
    strncpy(s->name, frameName, DBC_MAX_FRAME_NAME);
    s->name[DBC_MAX_FRAME_NAME - 1] = '\0';
    s->isMultiplexed = 0;

    HASH_ADD_INT(*db, canID, s);
}

Dbc_Frame_t *Dbc_FindFrame(Dbc_Frame_t *frame_list, int canID)
{
    Dbc_Frame_t *s;
    HASH_FIND_INT(frame_list, &canID, s);
    return s;
}

Dbc_Frame_t *Dbc_FindFrameByName(Dbc_Frame_t *frame_list, char *name)
{
    Dbc_Frame_t *s;

    for(s=frame_list; s != NULL; s=s->hh.next)
    {
        if(!strcmp(s->name, name))
        {
            return s;
        }
    }

    return NULL;
}

Dbc_Frame_t *Dbc_FindFrameBySignalname(Dbc_Frame_t *frame_list, char *name)
{
    Dbc_Frame_t *frame;
    Dbc_Signal_t *sig;

    for (frame=frame_list; frame != NULL; frame=frame->hh.next)
    {
        for (sig=frame->signals; sig != NULL; sig = sig->hh.next)
        {
            if(!strcmp(sig->name, name))
            {
                return frame;
            }
        }
    }

    return NULL;
}

Dbc_Signal_t *Dbc_FindSignalByName(Dbc_Frame_t *frame, char *name)
{
    Dbc_Signal_t *sig;

    for(sig=frame->signals; sig != NULL; sig = sig->hh.next)
    {
        if(!strcmp(sig->name, name))
        {
            return sig;
        }
    }

    return NULL;
}

const char *Dbc_FindValueString(Dbc_Signal_t *signal, int32_t value)
{
    Dbc_Value_t *v;
    HASH_FIND_INT(signal->values, &value, v);
    if (NULL == v)
    {
        return NULL;
    }

    return v->name;
}

void Dbc_AddSignal(
    Dbc_Frame_t *frame_list, int32_t frameId,
    char *signalName, int startBit, int signalLength,
    int is_big_endian, int signedState,
    float factor, float offset, float min, float max,
    char *unit,
    char *receiverList,
    uint8_t isMultiplexer, uint8_t muxId)
{
    Dbc_Frame_t *frame;
    Dbc_Signal_t *newSignal;

    frame = Dbc_FindFrame(frame_list, frameId);

    newSignal = malloc(sizeof(Dbc_Signal_t));
    strncpy(newSignal->name, signalName, DBC_MAX_SIGNAL_NAME);
    newSignal->name[DBC_MAX_SIGNAL_NAME - 1] = '\0';

    newSignal->startBit = startBit;
    newSignal->signalLength = signalLength;
    newSignal->is_big_endian = is_big_endian;
    newSignal->is_signed = signedState;
    newSignal->factor = factor;
    newSignal->offset = offset;
    newSignal->min = min;
    newSignal->max = max;
    newSignal->values = NULL;

    if(isMultiplexer > 0)
    {
        frame->isMultiplexed = 1;
    }
    newSignal->isMultiplexer = isMultiplexer;
    newSignal->muxId = muxId;

    strncpy(newSignal->unit, unit, DBC_MAX_UNIT_NAME);
    newSignal->unit[DBC_MAX_UNIT_NAME - 1] = '\0';

    strncpy(newSignal->receiverList, receiverList, DBC_MAX_RECEIVER_LIST);
    newSignal->receiverList[DBC_MAX_RECEIVER_LIST - 1] = '\0';

    HASH_ADD_STR(frame->signals, name, newSignal);
}

static int Dbc_ProcessStartBit(int byteOrder, int startBit, int signalLength)
{
    if (byteOrder == DBC_BO_BIG_ENDIAN)
    {
        int pos = 7 - (startBit % 8) + (signalLength - 1);
        if (pos < 8)
        {
            startBit = startBit - signalLength + 1;
        }
        else
        {
            int cpos = 7 - (pos % 8);
            int bytes = (int) (pos / 8);
            startBit = cpos + (bytes * 8) + (int) (startBit / 8) * 8;
        }
    }

    return startBit;
}

static void Dbc_ParseValues(Dbc_Frame_t **db, char *line)
{
    Dbc_Frame_t *frame;
    Dbc_Signal_t *signal;

    char *savePtr;
    /* Extract frame id */
    char *frameIdStr = strtok_r(&line[5], " \r\n", &savePtr);
    int frameId;
    int ret = sscanf(frameIdStr, "%d", &frameId);
    if (1 != ret)
    {
        fprintf(stderr, "Failed to read frame id: %s\n", frameIdStr);
        return;
    }

    /* Find the frame */
    frame = Dbc_FindFrame(*db, frameId);
    if (NULL == frame)
    {
        fprintf(stderr, "Failed to find a frame with id: %d\n", frameId);
        return;
    }

    /* Extract signal name */
    char *signalName = strtok_r(NULL, " \r\n", &savePtr);
    /* Find the signal */
    signal = Dbc_FindSignalByName(frame, signalName);

    char *token = signalName;

    for (;;)
    {
        /* Find numeric value */
        token = strtok_r(NULL, " \r\n", &savePtr);
        if (NULL == token)
        {
            /* No more values present, stop here */
            break;
        }
        if (';' == *token)
        {
            /* Line terminator found, stop here */
            break;
        }
        int value;
        ret = sscanf(token, "%d", &value);
        if (1 != ret)
        {
            fprintf(stderr, "Failed to parse value: '%s'\n", token);
            return;
        }

        /* Find string value */
        token = strtok_r(NULL, " \r\n", &savePtr);
        if (NULL == token)
        {
            fprintf(stderr, "Failed to find a string token\n");
            return;
        }

        Dbc_Value_t *newValue = malloc(sizeof(Dbc_Value_t));
        strncpy(newValue->name, token, DBC_MAX_VALUE_NAME);
        newValue->name[DBC_MAX_VALUE_NAME - 1] = '\0';
        newValue->value = value;

        HASH_ADD_INT(signal->values, value, newValue);

        const int len = strlen(newValue->name);
        if (';' == newValue->name[len - 1])
        {
            /* If we found a ';' symbol at the end - strip it and stop here */
            newValue->name[len - 1] = '\0';
            break;
        }
    }
}

int32_t Dbc_Init(Dbc_Frame_t **db, char *dbcFilePath)
{
    char line[DBC_MAX_LINE_SIZE];
    char frameName[DBC_MAX_FRAME_NAME], sender[DBC_MAX_SENDER_NAME];
    char signalName[DBC_MAX_SIGNAL_NAME], unit[DBC_MAX_UNIT_NAME], receiverList[DBC_MAX_RECEIVER_LIST];
    char signedState;

    int startBit = 0, signalLength = 0, byteOrder = 0;
    float factor = 0., offset = 0., min = 0., max = 0.;
    char mux[DBC_MAX_MUXLEN];
    uint8_t muxId = 0, isMultiplexer = 0;
    int frameId = 0, frameDlc, signalFound = 0, ret;

    FILE *fp = fopen(dbcFilePath, "r");
    if(NULL == fp)
    {
        fprintf(stderr, "Error opening %s\n", dbcFilePath);
        return -1;
    }

    while(fgets(line, DBC_MAX_LINE_SIZE - 1, fp))
    {
        /* Search for frames */
        if(sscanf(line, "BO_ %d %s %d %s", &frameId, frameName, &frameDlc, sender) == 4)
        {
            frameName[strlen(frameName) - 1] = '\0';  /* Remove last character ':' */
            Dbc_AddFrame(db, frameId, frameDlc, frameName);
        }
        /* Search for values */
        else if (strncmp(line, "VAL_ ", 5) == 0)
        {
            Dbc_ParseValues(db, line);
        }
        /* Search for signals */
        else
        {
            signalFound = 0;

            /* Check for a standard signal */
            ret = sscanf(line, " SG_ %s : %d|%d@%d%c (%f,%f) [%f|%f] %s %s", 
                signalName, &startBit, &signalLength, &byteOrder, &signedState, &factor, &offset, &min, &max, unit, receiverList );
            if(ret > DBC_MIN_SIGNAL_SCAN)
            {
                signalFound = 1;
                isMultiplexer = DBC_MUX_NONE;
                muxId = 0;
            }
            else
            {
                /* Check for a multiplexed signal */
                ret = sscanf(line, " SG_ %s %s : %d|%d@%d%c (%f,%f) [%f|%f] %s %s",
                    signalName, mux, &startBit, &signalLength, &byteOrder, &signedState, &factor, &offset, &min, &max, unit, receiverList );
                if(ret > DBC_MIN_SIGNAL_SCAN)
                {
                    signalFound = 1;

                    if(mux[0] == 'M')
                    {
                        isMultiplexer = DBC_MUX_DEFINE;
                        muxId = 0;
                    }
                    else if(mux[0] == 'm')
                    {
                        isMultiplexer = DBC_MUX_DATA;
                        sscanf(mux, "m%hhu", &muxId);
                    }
                    else
                    {
                        /* Shouldn't be here */
                        isMultiplexer = 0;
                        muxId = 0;
                    }

                }
            }

            if (signalFound)
            {
                /* Add signal to frame */
                startBit = Dbc_ProcessStartBit(byteOrder, startBit, signalLength);  /* Check for Big Endian */
                Dbc_AddSignal(*db, frameId, signalName, startBit, signalLength, byteOrder == DBC_BO_BIG_ENDIAN, signedState == '-',
                    factor, offset, min, max, unit, receiverList, isMultiplexer, muxId);
            }
        }
    }

    fclose(fp);
    return 0;
}

void Dbc_DeInit(Dbc_Frame_t *db)
{
    Dbc_Frame_t *frame, *frame_tmp;
    Dbc_Signal_t *signal, *signal_tmp;
    Dbc_Value_t *value, *value_tmp;

    HASH_ITER(hh, db, frame, frame_tmp)
    {
        HASH_ITER(hh, frame->signals, signal, signal_tmp)
        {
            HASH_ITER(hh, signal->values, value, value_tmp)
            {
                HASH_DEL(signal->values, value);
                free(value);
            }
            HASH_DEL(frame->signals, signal);
            free(signal);
        }
        HASH_DEL(db, frame);
        free(frame);
    }
}
