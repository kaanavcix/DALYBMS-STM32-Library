# DALY BMS STM32 Library

STM32 projelerinde DALY BMS modülleri ile UART üzerinden haberleşmek için hazırlanmış örnek kütüphane ve CubeIDE projesidir.

Bu örnek proje STM32F103C8TX için oluşturulmuştur. Kütüphane DALY BMS'e UART komutu gönderir, gelen 13 byte cevabı okur, checksum kontrolü yapar ve ölçüm değerlerini kullanımı kolay bir struct içine yazar.

## Özellikler

- UART üzerinden DALY BMS request-response haberleşmesi
- Toplam paket voltajı, akım ve SOC okuma
- Min/max hücre voltajı okuma
- Min/max sıcaklık okuma
- Charge/discharge MOS durumunu okuma
- Hücre sayısı, sıcaklık sensörü sayısı ve cycle bilgisi okuma
- Hücre voltajlarını çoklu frame olarak okuma
- Sıcaklık sensörlerini çoklu frame olarak okuma
- Charge MOS ve discharge MOS kontrol komutları
- BMS reset komutu

## Proje Yapısı

```text
Core/
  BMS/
    bms.c
    bms.h
    bms_types.h
  Inc/
  Src/
    main.c
Drivers/
dalybms_stm32.ioc
```

Ana kütüphane dosyaları `Core/BMS` klasöründedir.

## Donanım Bağlantısı

DALY BMS UART hattını STM32 UART hattına bağlayın.

| DALY BMS | STM32 |
| --- | --- |
| TX | UART RX |
| RX | UART TX |
| GND | GND |

Bu örnekte UART1 kullanılmaktadır.

Varsayılan UART ayarı:

```text
Baudrate : 9600
Data     : 8 bit
Parity   : None
Stop bit : 1
Mode     : TX/RX
```

> Not: BMS ve STM32 GND hattı ortak olmalıdır. Seviye dönüştürücü gerekip gerekmediğini kullandığınız BMS/UART adaptörüne göre kontrol edin.

## Hızlı Kullanım

`main.c` içinde örnek kullanım eklenmiştir:

```c
#include "bms.h"

static DALYBMSControlTypeDefInit dalyBms;
static bool dalyBmsReady = false;
static volatile bool dalyBmsLastUpdateOk = false;

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART1_UART_Init();

  dalyBmsReady = DALYBMS_Init(&dalyBms, huart1);

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
        float temp = dalyBms.bmsGetDatasTypeDefInit.tempAverage;

        (void)voltage;
        (void)current;
        (void)soc;
        (void)temp;
      }
    }

    HAL_Delay(1000);
  }
}
```

## Okunan Veriler

Veriler `DALYBMSControlTypeDefInit` içindeki `bmsGetDatasTypeDefInit` alanından okunur.

Örnek:

```c
float packVoltage = dalyBms.bmsGetDatasTypeDefInit.packVoltage;
float packCurrent = dalyBms.bmsGetDatasTypeDefInit.packCurrent;
float packSOC = dalyBms.bmsGetDatasTypeDefInit.packSOC;
float tempAverage = dalyBms.bmsGetDatasTypeDefInit.tempAverage;
uint8_t cellCount = dalyBms.bmsGetDatasTypeDefInit.numberOfCells;
```

Hücre voltajları:

```c
for (uint8_t i = 0; i < dalyBms.bmsGetDatasTypeDefInit.numberOfCells; i++)
{
  float cellVoltageMv = dalyBms.bmsGetDatasTypeDefInit.cellVmV[i];
  (void)cellVoltageMv;
}
```

Sıcaklık sensörleri:

```c
for (uint8_t i = 0; i < dalyBms.bmsGetDatasTypeDefInit.numOfTempSensors; i++)
{
  int16_t sensorTemp = dalyBms.bmsGetDatasTypeDefInit.cellTemperature[i];
  (void)sensorTemp;
}
```

## API Özeti

```c
bool DALYBMS_Init(DALYBMSControlTypeDefInit *dalyBMSControl, UART_HandleTypeDef huart);
bool DALYBMS_Update(DALYBMSControlTypeDefInit *dalyBMSControl);
```

`DALYBMS_Update()` temel okuma komutlarını sırayla çalıştırır. Başarılı olursa `true`, herhangi bir UART veya checksum hatasında `false` döner.

Tek tek okuma fonksiyonları:

```c
bool DALYBMS_GetPackMeasurements(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetMinMaxCellVoltage(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetPackTemperature(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetDischargeChargeMosStatus(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetStatusInfo(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALY_BMS_GetCellVoltages(DALYBMSControlTypeDefInit *dalyBMSControl);
bool DALYBMS_GetCellTemperature(DALYBMSControlTypeDefInit *dalyBMSControl);
```

Kontrol komutları:

```c
void DALYBMS_SetChargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl);
void DALYBMS_SetDischargeMOS(bool enabled, DALYBMSControlTypeDefInit *dalyBMSControl);
void DALYBMS_SetBmsReset(DALYBMSControlTypeDefInit *dalyBMSControl);
```

## Haberleşme Mantığı

Kütüphane normal UART transmit/receive mantığı ile çalışır.

1. STM32, DALY BMS'e 13 byte komut frame'i gönderir.
2. BMS 13 byte cevap frame'i döndürür.
3. Kütüphane start byte, adres, komut, payload length ve checksum kontrolü yapar.
4. Cevap doğruysa payload parse edilerek struct alanlarına yazılır.

Bu sürüm blocking UART kullanır:

```c
HAL_UART_Transmit(...)
HAL_UART_Receive(...)
```

DMA veya interrupt tabanlı UART kullanımı istenirse `sendCommand()` ve `receiveFrame()` fonksiyonları buna göre uyarlanabilir.

## Dikkat Edilecekler

- UART baudrate BMS ayarı ile aynı olmalıdır.
- `DALYBMS_Update()` birden fazla komut gönderdiği için cevap gelmeyen durumda timeout süresi kadar bekler.
- Hücre voltajlarını okumadan önce `DALY_BMS_GetStatusInfo()` çağrılmış olmalıdır. `DALYBMS_Update()` bunu otomatik yapar.
- Bu örnekte maksimum 48 hücre ve 16 sıcaklık sensörü desteklenir.
- MOS kontrol komutlarını gerçek batarya sisteminde dikkatli kullanın.

## STM32CubeIDE

Projeyi STM32CubeIDE ile açabilirsiniz. `Core/BMS` klasörü include path'e eklenmiştir.

Derleme sırasında `bms.h` bulunamazsa proje ayarlarından şu include path'i kontrol edin:

```text
../Core/BMS
```

## Lisans

Bu repo için ayrıca bir lisans dosyası eklenmediyse kullanım koşullarını proje sahibinin belirlemesi gerekir. STM32 HAL ve CMSIS dosyaları kendi lisansları ile gelir.
