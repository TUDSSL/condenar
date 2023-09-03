#ifndef SCREEN_FLASH
#define SCREEN_FLASH

#include <stdint.h>
#include <stdbool.h>

void Flash_Initialize();
void Flash_Write();
void Flash_Read();
void DoFlashWork();
void Flash_Write(uint32_t* address,void* dataPtr,uint32_t dataLength);
bool Flash_SelfTestDESTRUCTIVE();

#define MSPI_XIP_BASE_ADDRESS    0x18000000

#define EXT_FLASH_DOOM_WAD_ADDRESS MSPI_XIP_BASE_ADDRESS
#define EXT_FLASH_DOOM_WAD_LENGTH 4300800
#define EXT_FLASH_FILE_STORAGE_ADDRESS (EXT_FLASH_DOOM_WAD_ADDRESS + EXT_FLASH_DOOM_WAD_LENGTH)
#define EXT_FLASH_MAX_ADRESSS MSPI_XIP_BASE_ADDRESS + 0x3FFFFFF

#define SRAM_XIP_BASE_ADDRESS    0x10060000
#define FLASH_WAD_RESERVED_SPACE 0x00500000
#define EXT_RAM_BASE_ADDRESS 0x10160000

extern uint32_t lastSeenWADLength;

void Flash_Write_InternalAddress(uint32_t internalAddress,uint8_t* dataPtr,uint32_t dataLength);

void Flash_Print(int offset,int charsToPrint);

void Flash_Erase4KBBlock(uint32_t internalFlashAddress);
void Flash_EraseAll();
void Flash_EnableXIP();
void Flash_DisableXIP();

void CheckFlashAddress(void* address);


void InitFlashCaching();
#ifdef FLASH_CACHING_ENABLED
uint8_t* GetCacheableAddress(uint32_t address, int readSize);
#define GET_XIP_CHACHED_VALUE_8(addr) (*((volatile uint8_t *)(GetCacheableAddress(addr,1))))
#define GET_XIP_CHACHED_VALUE_16(addr) (*((volatile uint16_t *)(GetCacheableAddress(addr,2))))
#define GET_XIP_CHACHED_VALUE_32(addr) (*((volatile uint32_t *)(GetCacheableAddress(addr,4))))
#else 
#define GET_XIP_CHACHED_VALUE_8(addr) (*((volatile uint8_t *)(addr)))
#define GET_XIP_CHACHED_VALUE_16(addr) (*((volatile uint16_t *)(addr)))
#define GET_XIP_CHACHED_VALUE_32(addr) (*((volatile uint32_t *)(addr)))
#endif

#endif