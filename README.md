# DALYBMS STM32 Library

A small STM32 HAL library for communicating with DALY BMS modules over UART.

The library sends DALY BMS command frames, reads the 13-byte response frames, validates the checksum, and exposes the parsed pack data through a simple C struct.

## Features

- UART request-response communication with DALY BMS
- Pack voltage, current, and SOC readings
- Minimum and maximum cell voltage readings
- Minimum, maximum, and average temperature readings
- Charge/discharge MOS status readings
- Cell count, temperature sensor count, and cycle count readings
- Multi-frame cell voltage reading support
- Multi-frame temperature sensor reading support
- Charge MOS and discharge MOS control commands
- BMS reset command
- STM32CubeIDE example project included

## Repository Layout

```text
BMS/
  bms.c
  bms.h
  bms_types.h
example/
  Core/
    BMS/
    Inc/
    Src/
  Drivers/
  dalybms_stm32.ioc
```

`BMS/` contains the reusable library files.

`example/` contains an STM32F103C8TX STM32CubeIDE example project showing how to use the library with `USART1`.

## Hardware Connection

Connect the DALY BMS UART pins to the STM32 UART pins.

| DALY BMS | STM32 |
| --- | --- |
| TX | UART RX |
| RX | UART TX |
| GND | GND |

Default UART configuration used by the example:

```text
Baudrate : 9600
Data     : 8 bit
Parity   : None
Stop bit : 1
Mode     : TX/RX
```

Make sure the BMS and STM32 share the same ground. Check your BMS adapter voltage level before connecting it directly to the MCU.

## Quick Start

Include the library header:

```c
#include "bms.h"
```

Create a BMS control object:

```c
static DALYBMSControlTypeDefInit dalyBms;
static bool dalyBmsReady = false;
static volatile bool dalyBmsLastUpdateOk = false;
```

Initialize the library after UART initialization:

```c
MX_USART1_UART_Init();

dalyBmsReady = DALYBMS_Init(&dalyBms, huart1);
```

Read data periodically:

```c
while (1)
{
    if (dalyBmsReady)
    {
        dalyBmsLastUpdateOk = DALYBMS_Update(&dalyBms);

        if (dalyBmsLastUpdateOk)
        {
            float voltage = dalyBms.bmsGetDatasTypeDefInit.packVoltage;
            float current = dalyBms.bmsGetDatasTypeDefInit.packCurrent;
            float soc = dalyBms.bmsGetDatasTypeDefInit.packSOC;
            float temperature = dalyBms.bmsGetDatasTypeDefInit.tempAverage;

            (void)voltage;
            (void)current;
            (void)soc;
            (void)temperature;
        }
    }

    HAL_Delay(1000);
}
```

## API Overview

Main functions:

```c
bool DALYBMS_Init(DALYBMSControlTypeDefInit *dalyBMSControl, UART_HandleTypeDef huart);
bool DALYBMS_Update(DALYBMSControlTypeDefInit *dalyBMSControl);
```

Individual read functions:

```c
bool DALYBMS_GetPackMeasurements(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetMinMaxCellVoltage(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetPackTemperature(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetDischargeChargeMosStatus(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetStatusInfo(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetCellVoltages(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetCellTemperature(DALYBMSControlTypeDefInit *dalyBMSControl);
```

Control commands:

```c
void DALYBMS_SetChargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl);
void DALYBMS_SetDischargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl);
void DALYBMS_SetBmsReset(DALYBMSControlTypeDefInit *dalyBMSControl);
```

## Reading Parsed Data

After a successful `DALYBMS_Update()`, values are available in `bmsGetDatasTypeDefInit`.

```c
float packVoltage = dalyBms.bmsGetDatasTypeDefInit.packVoltage;
float packCurrent = dalyBms.bmsGetDatasTypeDefInit.packCurrent;
float packSOC = dalyBms.bmsGetDatasTypeDefInit.packSOC;
float tempAverage = dalyBms.bmsGetDatasTypeDefInit.tempAverage;
uint8_t numberOfCells = dalyBms.bmsGetDatasTypeDefInit.numberOfCells;
```

Cell voltages:

```c
for (uint8_t i = 0; i < dalyBms.bmsGetDatasTypeDefInit.numberOfCells; i++)
{
    float cellVoltageMv = dalyBms.bmsGetDatasTypeDefInit.cellVmV[i];
    (void)cellVoltageMv;
}
```

Temperature sensors:

```c
for (uint8_t i = 0; i < dalyBms.bmsGetDatasTypeDefInit.numOfTempSensors; i++)
{
    int16_t sensorTemperature = dalyBms.bmsGetDatasTypeDefInit.cellTemperature[i];
    (void)sensorTemperature;
}
```

## Communication Model

This library uses normal blocking STM32 HAL UART calls:

```c
HAL_UART_Transmit(...)
HAL_UART_Receive(...)
```

The flow is:

1. Send a 13-byte command frame to the BMS.
2. Read a 13-byte response frame.
3. Validate start byte, address, command, length, and checksum.
4. Parse the response payload into the data struct.

If the BMS does not respond, the read function returns `false` after the configured timeout.

## Notes

- The example project targets STM32F103C8TX.
- The library is written for STM32 HAL.
- The current implementation uses blocking UART calls, not DMA or interrupt mode.
- Avoid polling the BMS too aggressively. A 500 ms to 1000 ms update interval is usually a safer starting point.
- MOS control commands should be used carefully in real battery systems.

## License

No project-level license has been added yet. STM32 HAL and CMSIS files keep their own licenses.
