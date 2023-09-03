#include "GameboyAdvance.h"
#include "Kernel.h"
#include "TimeKeeper.h"
#include "Kernel_Draw.h"
#include "Helpers/Misc.h"
#include "stdlib.h"
#include "GBA.h"
#include "Cartridge.h"
#include "Kernel.h"
#include "FileSystem.h"
#include "Display.h"
#include "Helpers/PinDefinitions.h"
#include "Helpers/UART.h"

void GameboyAdvance_Update();
void GameboyAdvance_Render();

bool loadROM() {

	LOG_I("Initializing CPU memory...\n");
	if (!CPUInitMemory()) {
		return false;
	}

	uint8_t* biosPtr;
	if(!ShowFileSelection(GBA_BIOS, &biosPtr, NULL, NULL, NULL)) return false;

	LOG_I("Loading BIOS...\n");
	if (!CPULoadBios(biosPtr))	{
		return false;
	}

	uint8_t* romPtr;
	if(!ShowFileSelection(GBA, &romPtr, NULL, NULL, NULL)) return false;

	LOG_I("Loading ROM cartridge...\n");
	if (!cartridge_load_rom(romPtr))	{
		return false;
	}

	LOG_I("Initializing and resetting CPU...\n");
	CPUInit();
	CPUReset();

	return true;
}



void GameboyAdvanceEntryPoint(){

	LOG_I("Starting GBA emulator...\n");
	display_init();

    if(!loadROM()) {
		return;
	}

	//gamescreen_read_battery(game); //Write save data

    while(!k_getShouldCurrentAppExit()){
        GameboyAdvance_Update();
    }

	LOG_I("Shutting down GBA emulator...\n");
	//gamescreen_write_battery(game); //Write save data

	cartridge_unload();
}



void GameboyAdvance_Update(){
	k_input_state state;
	k_GetInput(&state, false);
	if(state.buttonB && state.triggerLeft && state.triggerRight){
		//k_setShouldCurrentAppExit(true);
	}    

	//LOG_I("Starting GBA CPULoop...");
	CPULoop(2500000);
	//LOG_I("Ended GBA CPULoop");
}


void GameboyAdvance_RenderFrame(){

}
