/*
 * bms.h
 *
 *  Created on: Aug 8, 2025
 *      Author: kaana
 */

#ifndef BMS_BMS_H_
#define BMS_BMS_H_

#include "bms_types.h"

bool DALYBMS_Init(DALYBMSControlTypeDefInit *dalyBMSControl, UART_HandleTypeDef huart);
bool DALYBMS_Update(DALYBMSControlTypeDefInit *dalyBMSControl);

bool DALYBMS_GetPackMeasurements(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetMinMaxCellVoltage(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetPackTemperature(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetDischargeChargeMosStatus(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetStatusInfo(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetCellVoltages(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetCellTemperature(DALYBMSControlTypeDefInit *dalyBMSControl);

void DALYBMS_SetDischargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl);
void DALYBMS_SetChargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl);
void DALYBMS_SetBmsReset(DALYBMSControlTypeDefInit *dalyBMSControl);

#endif /* BMS_BMS_H_ */
