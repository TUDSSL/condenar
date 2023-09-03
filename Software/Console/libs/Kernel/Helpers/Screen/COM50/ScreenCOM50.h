#include <stdint.h>
#include <stdbool.h>
#include "Screen_Base.h"

void SSD1963_ReadRegister(uint8_t registerNum, uint8_t* buffer, uint32_t lenght);
void SSD1963_WriteRegister(uint8_t registerNum, uint8_t* buffer, uint32_t lenght);
void SSD1963_WriteRegisterMemSet(uint8_t registerNum, uint8_t valueToSet, uint32_t lenght);
uint8_t SSD1963_ReadRegister8(uint8_t registerNum);
void SSD1963_WriteRegister8(uint8_t registerNum, uint8_t registerValue);
uint32_t SSD1963_ReadRegister32(uint8_t registerNum);
void SSD1963_WriteRegister32(uint8_t registerNum, uint32_t registerValue);

void COM50_WriteRegister8(uint8_t registerNum, uint8_t registerValue);

void SSD1963_SetupDrawRegion(uint32_t startX, uint32_t stopX, uint32_t startY, uint32_t stopY);
void SSD1963_SetToDrawRegion(bool resetStartingPoint, k_color colorToSet, uint32_t pixelsToSet);
void SSD1963_SendToDrawRegion(bool resetStartingPoint, k_color* dataToSend, uint32_t pixelsToSend);
void SSD1963_SendToDrawRegionAsync(bool resetStartingPoint, k_color* dataToSend, uint32_t pixelsToSend);

void COM50_SetBrightness(uint8_t brightness);
void COM50_SetContrast(uint8_t contrast);
void COM50_SetPowerMode(bool activeMode);

void SSD1963_WaitForAsync();

void SSD1963_Initialize();
void COM50_Initialize();