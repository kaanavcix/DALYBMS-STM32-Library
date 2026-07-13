/*
 * bms.c
 *
 *  Created on: Aug 8, 2025
 *      Author: kaana
 */

#include "bms.h"
#include <string.h>

#define READ_BIT_VALUE(value, bit) (((value) >> (bit)) & 0x01U)

static uint16_t readU16(const uint8_t *buffer, uint8_t index)
{
    return ((uint16_t)buffer[index] << 8) | (uint16_t)buffer[index + 1U];
}

static uint32_t readU32(const uint8_t *buffer, uint8_t index)
{
    return ((uint32_t)buffer[index] << 24) |
           ((uint32_t)buffer[index + 1U] << 16) |
           ((uint32_t)buffer[index + 2U] << 8) |
           (uint32_t)buffer[index + 3U];
}

static uint8_t calculateChecksum(const uint8_t *buffer)
{
    uint8_t checksum = 0U;

    for (uint8_t i = 0U; i < (DALYBMS_FRAME_LENGTH - 1U); i++)
        checksum += buffer[i];

    return checksum;
}

static void prepareCommand(BMSCOMMANDTYPE command, const uint8_t payload[DALYBMS_PAYLOAD_LENGTH],
                           DALYBMSControlTypeDefInit *dalyBMSControl)
{
    memset(dalyBMSControl->bms_txBuffer, 0, DALYBMS_FRAME_LENGTH);

    dalyBMSControl->bmsCommandEnum = command;
    dalyBMSControl->bms_txBuffer[0] = DALYBMS_START_BYTE;
    dalyBMSControl->bms_txBuffer[1] = DALYBMS_HOST_ADDRESS;
    dalyBMSControl->bms_txBuffer[2] = (uint8_t)command;
    dalyBMSControl->bms_txBuffer[3] = DALYBMS_PAYLOAD_LENGTH;

    if (payload != NULL)
        memcpy(&dalyBMSControl->bms_txBuffer[4], payload, DALYBMS_PAYLOAD_LENGTH);

    dalyBMSControl->bms_txBuffer[DALYBMS_FRAME_LENGTH - 1U] =
        calculateChecksum(dalyBMSControl->bms_txBuffer);
}

static bool sendCommand(BMSCOMMANDTYPE command, const uint8_t payload[DALYBMS_PAYLOAD_LENGTH],
                        DALYBMSControlTypeDefInit *dalyBMSControl)
{
    __HAL_UART_CLEAR_OREFLAG(&dalyBMSControl->huart);
    (void)__HAL_UART_FLUSH_DRREGISTER(&dalyBMSControl->huart);

    prepareCommand(command, payload, dalyBMSControl);

    return HAL_UART_Transmit(&dalyBMSControl->huart, dalyBMSControl->bms_txBuffer,
                             DALYBMS_FRAME_LENGTH, DALYBMS_DEFAULT_TIMEOUT_MS) == HAL_OK;
}

static bool validateFrame(BMSCOMMANDTYPE command, DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (dalyBMSControl->bms_rxBuffer[0] != DALYBMS_START_BYTE)
        return false;

    if (dalyBMSControl->bms_rxBuffer[1] != DALYBMS_RESPONSE_ADDRESS)
        return false;

    if (dalyBMSControl->bms_rxBuffer[2] != (uint8_t)command)
        return false;

    if (dalyBMSControl->bms_rxBuffer[3] != DALYBMS_PAYLOAD_LENGTH)
        return false;

    return calculateChecksum(dalyBMSControl->bms_rxBuffer) ==
           dalyBMSControl->bms_rxBuffer[DALYBMS_FRAME_LENGTH - 1U];
}

static bool receiveFrame(BMSCOMMANDTYPE command, DALYBMSControlTypeDefInit *dalyBMSControl)
{
    memset(dalyBMSControl->bms_rxBuffer, 0, DALYBMS_FRAME_LENGTH);

    if (HAL_UART_Receive(&dalyBMSControl->huart, dalyBMSControl->bms_rxBuffer,
                         DALYBMS_FRAME_LENGTH, DALYBMS_DEFAULT_TIMEOUT_MS) != HAL_OK)
        return false;

    return validateFrame(command, dalyBMSControl);
}

static bool requestFrame(BMSCOMMANDTYPE command, DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!sendCommand(command, NULL, dalyBMSControl))
        return false;

    return receiveFrame(command, dalyBMSControl);
}

static bool isValidCellCount(uint8_t cellCount)
{
    return cellCount >= MIN_NUMBER_CELLS && cellCount <= MAX_NUMBER_CELLS;
}

static bool isValidTempSensorCount(uint8_t sensorCount)
{
    return sensorCount >= MIN_NUMBER_TEMP_SENSORS && sensorCount <= MAX_NUMBER_TEMP_SENSORS;
}

bool DALYBMS_Init(DALYBMSControlTypeDefInit *dalyBMSControl, UART_HandleTypeDef huart)
{
    if (dalyBMSControl == NULL)
        return false;

    memset(dalyBMSControl, 0, sizeof(*dalyBMSControl));
    dalyBMSControl->huart = huart;

    return HAL_UART_Init(&dalyBMSControl->huart) == HAL_OK;
}

bool DALYBMS_Update(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!DALYBMS_GetPackMeasurements(dalyBMSControl))
        return false;

    if (!DALY_BMS_GetMinMaxCellVoltage(dalyBMSControl))
        return false;

    if (!DALYBMS_GetPackTemperature(dalyBMSControl))
        return false;

    if (!DALYBMS_GetDischargeChargeMosStatus(dalyBMSControl))
        return false;

    if (!DALY_BMS_GetStatusInfo(dalyBMSControl))
        return false;

    if (!DALY_BMS_GetCellVoltages(dalyBMSControl))
        return false;

    return DALYBMS_GetCellTemperature(dalyBMSControl);
}

bool DALYBMS_GetPackMeasurements(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!requestFrame(VOUT_IOUT_SOC, dalyBMSControl))
        return false;

    dalyBMSControl->bmsGetDatasTypeDefInit.packVoltage = (float)readU16(dalyBMSControl->bms_rxBuffer, 4U) / 10.0f;
    dalyBMSControl->bmsGetDatasTypeDefInit.gatherPackVoltage = (float)readU16(dalyBMSControl->bms_rxBuffer, 6U) / 10.0f;
    dalyBMSControl->bmsGetDatasTypeDefInit.packCurrent = ((float)readU16(dalyBMSControl->bms_rxBuffer, 8U) - 30000.0f) / 10.0f;
    dalyBMSControl->bmsGetDatasTypeDefInit.packSOC = (float)readU16(dalyBMSControl->bms_rxBuffer, 10U) / 10.0f;

    return true;
}

bool DALY_BMS_GetMinMaxCellVoltage(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!requestFrame(MIN_MAX_CELL_VOLTAGE, dalyBMSControl))
        return false;

    dalyBMSControl->bmsGetDatasTypeDefInit.maxCellmV = (float)readU16(dalyBMSControl->bms_rxBuffer, 4U);
    dalyBMSControl->bmsGetDatasTypeDefInit.maxCellVNum = dalyBMSControl->bms_rxBuffer[6];
    dalyBMSControl->bmsGetDatasTypeDefInit.minCellmV = (float)readU16(dalyBMSControl->bms_rxBuffer, 7U);
    dalyBMSControl->bmsGetDatasTypeDefInit.minCellVNum = dalyBMSControl->bms_rxBuffer[9];
    dalyBMSControl->bmsGetDatasTypeDefInit.cellDiff =
        dalyBMSControl->bmsGetDatasTypeDefInit.maxCellmV - dalyBMSControl->bmsGetDatasTypeDefInit.minCellmV;

    return true;
}

bool DALYBMS_GetPackTemperature(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!requestFrame(MIN_MAX_TEMPERATURE, dalyBMSControl))
        return false;

    dalyBMSControl->bmsGetDatasTypeDefInit.tempMax = (int16_t)dalyBMSControl->bms_rxBuffer[4] - 40;
    dalyBMSControl->bmsGetDatasTypeDefInit.tempMin = (int16_t)dalyBMSControl->bms_rxBuffer[6] - 40;
    dalyBMSControl->bmsGetDatasTypeDefInit.tempAverage =
        ((float)dalyBMSControl->bmsGetDatasTypeDefInit.tempMax +
         (float)dalyBMSControl->bmsGetDatasTypeDefInit.tempMin) / 2.0f;

    return true;
}

bool DALYBMS_GetDischargeChargeMosStatus(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!requestFrame(DISCHARGE_CHARGE_MOS_STATUS, dalyBMSControl))
        return false;

    switch (dalyBMSControl->bms_rxBuffer[4])
    {
    case 1:
        strcpy(dalyBMSControl->bmsGetDatasTypeDefInit.chargeDischargeStatus, "Charge");
        break;
    case 2:
        strcpy(dalyBMSControl->bmsGetDatasTypeDefInit.chargeDischargeStatus, "Discharge");
        break;
    default:
        strcpy(dalyBMSControl->bmsGetDatasTypeDefInit.chargeDischargeStatus, "Stationary");
        break;
    }

    dalyBMSControl->bmsGetDatasTypeDefInit.chargeFetState = dalyBMSControl->bms_rxBuffer[5] != 0U;
    dalyBMSControl->bmsGetDatasTypeDefInit.disChargeFetState = dalyBMSControl->bms_rxBuffer[6] != 0U;
    dalyBMSControl->bmsGetDatasTypeDefInit.bmsHeartBeat = dalyBMSControl->bms_rxBuffer[7];
    dalyBMSControl->bmsGetDatasTypeDefInit.resCapacitymAh = readU32(dalyBMSControl->bms_rxBuffer, 8U);

    return true;
}

bool DALY_BMS_GetStatusInfo(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    if (!requestFrame(STATUS_INFO, dalyBMSControl))
        return false;

    dalyBMSControl->bmsGetDatasTypeDefInit.numberOfCells = dalyBMSControl->bms_rxBuffer[4];
    dalyBMSControl->bmsGetDatasTypeDefInit.numOfTempSensors = dalyBMSControl->bms_rxBuffer[5];
    dalyBMSControl->bmsGetDatasTypeDefInit.chargeState = dalyBMSControl->bms_rxBuffer[6] != 0U;
    dalyBMSControl->bmsGetDatasTypeDefInit.loadState = dalyBMSControl->bms_rxBuffer[7] != 0U;

    for (uint8_t i = 0U; i < 8U; i++)
        dalyBMSControl->bmsGetDatasTypeDefInit.dIO[i] = READ_BIT_VALUE(dalyBMSControl->bms_rxBuffer[8], i) != 0U;

    dalyBMSControl->bmsGetDatasTypeDefInit.bmsCycles = readU16(dalyBMSControl->bms_rxBuffer, 9U);

    return isValidCellCount(dalyBMSControl->bmsGetDatasTypeDefInit.numberOfCells) &&
           isValidTempSensorCount(dalyBMSControl->bmsGetDatasTypeDefInit.numOfTempSensors);
}

bool DALY_BMS_GetCellVoltages(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    uint8_t cellCount = dalyBMSControl->bmsGetDatasTypeDefInit.numberOfCells;
    uint8_t cellNo = 0U;
    uint8_t packetCount;

    if (!isValidCellCount(cellCount))
        return false;

    packetCount = (uint8_t)((cellCount + 2U) / 3U);

    if (!sendCommand(CELL_VOLTAGES, NULL, dalyBMSControl))
        return false;

    for (uint8_t packet = 0U; packet < packetCount; packet++)
    {
        if (!receiveFrame(CELL_VOLTAGES, dalyBMSControl))
            return false;

        for (uint8_t i = 0U; i < 3U && cellNo < cellCount; i++)
        {
            dalyBMSControl->bmsGetDatasTypeDefInit.cellVmV[cellNo] =
                (float)readU16(dalyBMSControl->bms_rxBuffer, (uint8_t)(5U + (2U * i)));
            cellNo++;
        }
    }

    return true;
}

bool DALYBMS_GetCellTemperature(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    uint8_t sensorCount = dalyBMSControl->bmsGetDatasTypeDefInit.numOfTempSensors;
    uint8_t sensorNo = 0U;
    uint8_t packetCount;

    if (!isValidTempSensorCount(sensorCount))
        return false;

    packetCount = (uint8_t)((sensorCount + 6U) / 7U);

    if (!sendCommand(CELL_TEMPERATURE, NULL, dalyBMSControl))
        return false;

    for (uint8_t packet = 0U; packet < packetCount; packet++)
    {
        if (!receiveFrame(CELL_TEMPERATURE, dalyBMSControl))
            return false;

        for (uint8_t i = 0U; i < 7U && sensorNo < sensorCount; i++)
        {
            dalyBMSControl->bmsGetDatasTypeDefInit.cellTemperature[sensorNo] =
                (int16_t)dalyBMSControl->bms_rxBuffer[5U + i] - 40;
            sensorNo++;
        }
    }

    return true;
}

void DALYBMS_SetDischargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl)
{
    uint8_t payload[DALYBMS_PAYLOAD_LENGTH] = {0};

    payload[0] = enabled ? 1U : 0U;
    (void)sendCommand(DISCHRG_FET, payload, dalyBMSControl);
}

void DALYBMS_SetChargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl)
{
    uint8_t payload[DALYBMS_PAYLOAD_LENGTH] = {0};

    payload[0] = enabled ? 1U : 0U;
    (void)sendCommand(CHRG_FET, payload, dalyBMSControl);
}

void DALYBMS_SetBmsReset(DALYBMSControlTypeDefInit *dalyBMSControl)
{
    (void)sendCommand(BMS_RESET, NULL, dalyBMSControl);
}
