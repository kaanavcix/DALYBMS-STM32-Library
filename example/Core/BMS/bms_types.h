/*
 * bms_types.h
 *
 *  Created on: Aug 9, 2025
 *      Author: kaana
 */

#ifndef BMS_BMS_TYPES_H_
#define BMS_BMS_TYPES_H_

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#define DALYBMS_FRAME_LENGTH             13U
#define DALYBMS_PAYLOAD_LENGTH           8U
#define DALYBMS_START_BYTE               0xA5U
#define DALYBMS_HOST_ADDRESS             0x40U
#define DALYBMS_RESPONSE_ADDRESS         0x01U
#define DALYBMS_DEFAULT_TIMEOUT_MS       1000U

#define MIN_NUMBER_CELLS                 1U
#define MAX_NUMBER_CELLS                 48U
#define MIN_NUMBER_TEMP_SENSORS          1U
#define MAX_NUMBER_TEMP_SENSORS          16U

#define XFER_BUFFER_LENGTH               DALYBMS_FRAME_LENGTH

typedef enum BMSCOMMAND
{
    VOUT_IOUT_SOC = 0x90,
    MIN_MAX_CELL_VOLTAGE = 0x91,
    MIN_MAX_TEMPERATURE = 0x92,
    DISCHARGE_CHARGE_MOS_STATUS = 0x93,
    STATUS_INFO = 0x94,
    CELL_VOLTAGES = 0x95,
    CELL_TEMPERATURE = 0x96,
    CELL_BALANCE_STATE = 0x97,
    FAILURE_CODES = 0x98,
    DISCHRG_FET = 0xD9,
    CHRG_FET = 0xDA,
    BMS_RESET = 0x00
} BMSCOMMANDTYPE;

typedef struct BMSGetDatasTypeDef
{
    /* 0x90 - Total voltage, gather voltage, current and SOC */
    float packVoltage;
    float gatherPackVoltage;
    float packCurrent;
    float packSOC;

    /* 0x91 - Min/max cell voltage */
    float maxCellmV;
    uint8_t maxCellVNum;
    float minCellmV;
    uint8_t minCellVNum;
    float cellDiff;

    /* 0x92 - Min/max temperature */
    int16_t tempMax;
    int16_t tempMin;
    float tempAverage;

    /* 0x93 - MOS and remaining capacity */
    char chargeDischargeStatus[12];
    bool chargeFetState;
    bool disChargeFetState;
    uint8_t bmsHeartBeat;
    uint32_t resCapacitymAh;

    /* 0x94 - Pack status */
    uint8_t numberOfCells;
    uint8_t numOfTempSensors;
    bool chargeState;
    bool loadState;
    bool dIO[8];
    uint16_t bmsCycles;

    /* 0x95 - Cell voltages */
    float cellVmV[MAX_NUMBER_CELLS];

    /* 0x96 - Temperature sensors */
    int16_t cellTemperature[MAX_NUMBER_TEMP_SENSORS];

    /* 0x97 - Cell balance status */
    bool cellBalanceState[MAX_NUMBER_CELLS];
    bool cellBalanceActive;
} BMSGetDatasTypeDefInit;

typedef struct DALYBMSControlTypeDef
{
    UART_HandleTypeDef huart;
    BMSCOMMANDTYPE bmsCommandEnum;
    BMSGetDatasTypeDefInit bmsGetDatasTypeDefInit;
    uint8_t bms_txBuffer[DALYBMS_FRAME_LENGTH];
    uint8_t bms_rxBuffer[DALYBMS_FRAME_LENGTH];
} DALYBMSControlTypeDefInit;

#endif /* BMS_BMS_TYPES_H_ */
