/**
 * @file dbc.h
 *
 * DBC handler
 */

#ifndef DBC_H
#define DBC_H

/**
Section: Included Files
*/

#include "uthash.h"

#include <net/if.h>
#include <linux/can.h>
#include "lib.h"

/**
Section: Definitions
*/

#define DBC_MAX_LINE_SIZE     512
#define DBC_MAX_FRAME_NAME    80
#define DBC_MAX_SENDER_NAME   128
#define DBC_MAX_SIGNAL_NAME   80
#define DBC_MAX_UNIT_NAME     80
#define DBC_MAX_RECEIVER_LIST 256
#define DBC_MAX_MUXLEN        4
#define DBC_MIN_SIGNAL_SCAN   5

/**
Section: Public Types
*/

typedef enum
{
	DBC_BO_BIG_ENDIAN = 0,
	DBC_BO_LITTLE_ENDIAN = 1
} Dbc_Endianness_t;

typedef enum
{
	DBC_MUX_NONE = 0,
	DBC_MUX_DEFINE = 1,
	DBC_MUX_DATA = 2
} Dbc_Mux_t;

typedef struct
{
	char name[DBC_MAX_SIGNAL_NAME];
	int startBit;
	int signalLength;
	int is_big_endian;  /**< Intel = 0; Motorola (== BIG Endian) = 1 */
	int is_signed;
	float factor;
	float offset;
	float min;
	float max;
	char unit[DBC_MAX_UNIT_NAME];
	char receiverList[DBC_MAX_RECEIVER_LIST];
    uint8_t isMultiplexer;
    uint8_t muxId;
	uint8_t number;

	UT_hash_handle hh;
} Dbc_Signal_t;

typedef struct
{
	int32_t canID;
	uint8_t dlc;
	char name[DBC_MAX_FRAME_NAME];
    uint8_t isMultiplexed;
	Dbc_Signal_t *signals;

	UT_hash_handle hh;
} Dbc_Frame_t;

/**
Section: Public Function Declarations
*/

/**
 * @brief      Reads a .dbc file and extracts all info in db parameter
 *
 * @param      db[out]          The database
 * @param      dbcFilePath[in]  The dbc file path
 *
 * @return     0 on success and a negative value otherwise
 */
int32_t Dbc_Init(Dbc_Frame_t **db, char *dbcFilePath);

/**
 * @brief      Free all memory taken by frame and signal descriptions
 *
 * @param[in]  db    The database
 */
void Dbc_DeInit(Dbc_Frame_t *db);

Dbc_Frame_t *Dbc_FindFrame(Dbc_Frame_t *frame_list, int canID);
Dbc_Frame_t *Dbc_FindFrameByName(Dbc_Frame_t *frame_list, char *name);
Dbc_Frame_t *Dbc_FindFrameBySignalname(Dbc_Frame_t *frame_list, char *name);

Dbc_Signal_t *Dbc_FindSignalByName(Dbc_Frame_t *frame, char *name);

void Dbc_AddFrame(Dbc_Frame_t **db, int32_t canID, uint8_t dlc, char *frameName);
void Dbc_AddSignal(
    Dbc_Frame_t *frame_list, int32_t frameId,
    char *signalName, int startBit, int signalLength,
    int is_big_endian, int signedState,
    float factor, float offset, float min, float max,
    char *unit,
    char *receiverList,
    unsigned char isMultiplexer, unsigned char muxId);

#endif  /* DBC_H */
