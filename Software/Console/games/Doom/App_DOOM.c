#include "App_DOOM.h"
#include "Helpers/Misc.h"
#include "Helpers/Screen/Screen.h"
#include "Helpers/PinDefinitions.h"
#include "d_main.h"
#include "global_data.h"
#include "z_zone.h"
#include "Helpers/UART.h"
#include "Helpers/TimeKeeper.h"
#include "Helpers/Flash.h"
#include "Kernel.h"
#include "Kernel_Draw.h"

void initImmutableFlashRegion();

void CalculateWADChecksum(){ 
    uint32_t lastSeenWADLength= *((uint32_t*)MSPI_XIP_BASE_ADDRESS);
    LOG_I("WAD file size: %d",lastSeenWADLength);

    uint32_t startTime = micros();   
    uint32_t chekcsum=CalculateChecksum((uint8_t*)MSPI_XIP_BASE_ADDRESS,lastSeenWADLength+4);
    uint32_t endTime = micros();
    LOG_I("WAD file checksum: %d. Done in %.2f seconds",chekcsum,(endTime-startTime)/1000000.0f);
}

void DOOMEntryPoint(k_app* app){    
    printf("Starting Doom port to APOLLO4\r\n");

    k_screen_RenderLoadingScreen(0,"Verifying WAD integrity...");
    CalculateWADChecksum();

    LOG_I("Setup finished, starting main game loop...");

    memset((void*)SRAM_XIP_BASE_ADDRESS,0xFF,1024*1024);

    Z_Init();
    InitGlobals();

    _g->gamma = k_GetSettingUInt32("/DOOM/Brightness",2);
    _g->gamma = 2;

    D_DoomMain();  

	
}


void D_DoomMainSetup();
void P_SetupLevelFromCheckpoint();
void G_DoLoadLevel();

void DOOMOnCheckpointRestore(){
    //Execute parts of the setup functions that write things to the SRAM 1MB cache
    D_DoomMainSetup();
    P_SetupLevelFromCheckpoint();
   _g->forceSTRefresh=true;
}

