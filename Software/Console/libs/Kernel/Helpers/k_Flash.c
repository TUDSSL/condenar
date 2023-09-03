#include "Flash.h"
#include "Helpers/Misc.h"
#include "Helpers/PinDefinitions.h"
#include "hal/am_hal_gpio.h"
#include "am_mcu_apollo.h"
#include "Misc.h"
#include <string.h>
#include "hal/am_hal_gpio.h"
#include "am_bsp.h"
#include "RNG.h"
#include "TimeKeeper.h"


#define MPSI_TEST_PAGE_INCR     (AM_DEVICES_MSPI_FLASH_PAGE_SIZE*3)
#define MSPI_SECTOR_INCR        (33)
#define MSPI_INT_TIMEOUT        (100)

#define MSPI_TARGET_SECTOR      (16)
#define MSPI_BUFFER_SIZE        (4*1024)  // 16K example buffer size.

#define DEFAULT_TIMEOUT         10000

#define MSPI_TEST_MODULE        1


#define AM_DEVICES_MSPI_FLASH_SECTOR_SIZE 4



//#define START_SPEED_INDEX       0
//#define END_SPEED_INDEX         11

uint8_t testGeneratorBuffer[1024];
uint8_t testComparerBuffer[1024];

#include "am_devices_mspi_s25fl256l.h"




void Flash_Initialize(){   

    LOG_I("Initializing flash...");

    GPIO_ModeOut(PIN_ENABLE_MEM_VCC);
    GPIO_SetLevel(PIN_ENABLE_MEM_VCC,1);
    DelayMS(10);

    const am_hal_gpio_pincfg_t g_ADC_PIN_CFG = { .GP.cfg_b.uFuncSel = AM_HAL_PIN_68_NCE68, .GP.cfg_b.uNCE=FLASH_NCE_NUM };//The NCE number is undocumented, this one works for now
    am_hal_gpio_pinconfig(PIN_FLASH_CS, g_ADC_PIN_CFG);
    
    uint32_t ui32Status = am_devices_mspi_s25fl256l_init(AM_HAL_MSPI_CLK_32MHZ, true);        
    ui32Status|=am_devices_mspi_s25fl256l_id();


    if(ui32Status!=0){
        LOG_E("Failed to initialize the external Flash memory!");
    }else{
        LOG_I("Flash initialized OK!");
    }

  
  #if FLASH_MSPI_NUM == 0
    NVIC_EnableIRQ(MSPI0_IRQn);
    #elif FLASH_MSPI_NUM == 1
    NVIC_EnableIRQ(MSPI1_IRQn);
    #elif FLASH_MSPI_NUM == 2
    NVIC_EnableIRQ(MSPI2_IRQn);
    #else
    #error "Invalid MSPI num"
    #endif
    
    am_hal_interrupt_master_enable();

    Flash_EnableXIP();

    InitFlashCaching();

    //Flash_SelfTestDESTRUCTIVE();

}

bool Flash_SelfTestDESTRUCTIVE(){       

    LOG_I("Starting destructive flash test...");
    Flash_DisableXIP();

    //Generate test data
  //  int startingAddress=16;
  //  int testSize=sizeof(testGeneratorBuffer)-startingAddress;
     int startingAddress=65999;
    int testSize=875;
    for (uint32_t i = 0; i < testSize; i++)
    {
        testGeneratorBuffer[i] = GetRandomInt();
    }
    
    uint32_t addressToWrite= startingAddress + MSPI_XIP_BASE_ADDRESS;
    uint8_t *pui8Address = (uint8_t *)(addressToWrite);

    //Erase and write test data
    Flash_Erase4KBBlock(addressToWrite-MSPI_XIP_BASE_ADDRESS);
    Flash_Write((uint32_t*)addressToWrite,testGeneratorBuffer,testSize);
    
    // Enable XIP mode to read everything back.    
    Flash_EnableXIP();

    //am_hal_cachectrl_enable();
    /*am_hal_daxi_config_t am_hal_daxi_defaults =
    {
        .agingCounter             = 2,
        .eNumBuf                  = AM_HAL_DAXI_CONFIG_NUMBUF_4,
        .eNumFreeBuf              = AM_HAL_DAXI_CONFIG_NUMFREEBUF_3,
    };*/
 


    uint32_t startTime=micros();


    for (uint32_t i = 0; i < testSize; i++)
    {        
        if(pui8Address[i]!=testGeneratorBuffer[i]){
            LOG_I("BYTE flash test check failed at index %d.",i);
            return false;
        }
    }

    uint32_t totalTime=micros()-startTime;
    LOG_I("BYTE flash test of size %d passed in %dms. (%.2f Kb/s)", testSize,totalTime/1000,(8*testSize/1024.0f)/(totalTime/1000000.0f));


    startTime=micros();
    for (uint32_t i = 0; i < testSize/4; i++)
    {        
        if(pui8Address[i*4]!=testGeneratorBuffer[i*4]){
           
            LOG_I("INT flash test check failed at index %d.",i);
            return false;
        }
    }
    totalTime=micros()-startTime;
    LOG_I("INT flash test of size %d passed in %dms. (%.2f Kb/s)", testSize,totalTime/1000,(8*testSize/1024.0f)/(totalTime/1000000.0f));
    

    startTime=micros();
    memcpy(testComparerBuffer,pui8Address,testSize);
    if(memcmp(testComparerBuffer,testGeneratorBuffer,testSize)!=0) {
        LOG_I("MEMCPY Flash test check failed.");
        return false;
    }
    totalTime=micros()-startTime;
    LOG_I("MEMCPY flash test of size %d passed in %dms. (%.2f Kb/s)", testSize,totalTime/1000,(8*testSize/1024.0f)/(totalTime/1000000.0f));

    return true;
}

void Flash_Write(uint32_t* address,void* dataPtr,uint32_t dataLength){
    CheckFlashAddress(address);  

    uint32_t internalAddress=(uint32_t)address - MSPI_XIP_BASE_ADDRESS;


    uint32_t bytesRemaining=dataLength;


    while(bytesRemaining>0){
        uint32_t bytesToProgramInThisPage = 0x100 - (internalAddress & 0xFF);
        if(bytesToProgramInThisPage > bytesRemaining) bytesToProgramInThisPage= bytesRemaining;

        //LOG_I("Programming %d bytes",bytesToProgramInThisPage);
        Flash_Write_InternalAddress(internalAddress,dataPtr, bytesToProgramInThisPage);

        internalAddress+=bytesToProgramInThisPage;
        dataPtr+=bytesToProgramInThisPage;
        bytesRemaining-=bytesToProgramInThisPage;
    }

    
}

void Flash_Write_InternalAddress(uint32_t internalAddress,uint8_t* dataPtr,uint32_t dataLength){
    uint32_t ui32Status =  am_devices_mspi_s25fl256l_write(dataPtr,internalAddress,dataLength,true);
    if (AM_HAL_STATUS_SUCCESS != ui32Status){
        LOG_W("Failed to write %d bytes to address %d!",dataLength,internalAddress);
    }    
}


void Flash_Erase4KBBlock(uint32_t internalFlashAddress)
{
    CheckFlashAddress((uint8_t*)internalFlashAddress+MSPI_XIP_BASE_ADDRESS);

    uint32_t ui32Status = am_devices_mspi_s25fl256l_sector_erase(internalFlashAddress);
    if (AM_HAL_STATUS_SUCCESS != ui32Status){
        LOG_W("Failed to erase 4KB block!");
    }    
}

void Flash_EraseAll()
{
    uint32_t ui32Status = am_devices_mspi_s25fl256l_mass_erase();
        if (AM_HAL_STATUS_SUCCESS != ui32Status){
        LOG_W("Failed to erase All flash!");
    }       
}

void Flash_Print(int offset,int charsToPrint){
    uint8_t* flashAddr=(uint8_t*)(MSPI_XIP_BASE_ADDRESS+offset);

    for(int i=0;i<charsToPrint;i++){
        am_util_stdio_printf("%c",flashAddr[i]);
    }
    am_util_stdio_printf("\r\n");
}

void Flash_EnableXIP(){        
    if (am_devices_mspi_s25fl256l_enable_xip() != 0)
    {
        LOG_I("Failed to enable XIP mode in the Flash!\n");
    } 
}

void Flash_DisableXIP(){
    if (am_devices_mspi_s25fl256l_disable_xip() != 0)
    {
        LOG_I("Failed to disable XIP mode in the Flash!\n");
    } 
}

void CheckFlashAddress(void* address){
    if((uint8_t*)address<(uint8_t*)MSPI_XIP_BASE_ADDRESS){
        LOG_E("CheckFlashAddress failed due to a too low address");
        SpinLoop();
    }

    if((uint8_t*)address>(uint8_t*)MSPI_XIP_BASE_ADDRESS+0x7FFFFF){ //Max address we support (8M)
        LOG_E("CheckFlashAddress failed due to a too high address");
        SpinLoop();
    }

  /*  if(protectAgainstWADoverrides){
        uint32_t wadLength=lastSeenWADLength; //The length is stored there
        if((uint8_t*)address<(uint8_t*)MSPI_XIP_BASE_ADDRESS+wadLength){
            LOG_E("CheckFlashAddress failed due to writing in the WAD protected zone!");
            SpinLoop();
        }
    }*/
}