#include "App_DOOM.h"
#include "MainMenu.h"
#include "TestMode.h"
#include "Settings.h"
#include "GameboyAdvance.h"
#include "Kernel.h"
#include "SettingsHandler.h"
#include "DownloadMode.h"
#include "Helpers/FileSystem.h"
#include "GameboyColor.h"
#include "TurnOffApp.h"

k_app doom = {
    .name = "Doom",
    .entryPoint = &DOOMEntryPoint,
    .onCheckpointRestore = &DOOMOnCheckpointRestore,
    .appIconImageName = "doomIcon",
    .requestedStorageBytes = 8 * 1000 //8KB of storage for DOOM
};


k_app gameboyColor = {
    .name = "GameBoy",
    .entryPoint = &GameboyColorEntryPoint,
    .onCheckpointRestore = &GBOnCheckpointRestore,
    .requestedStorageBytes = 8*1024, //8KB of storage for Gameboy
    .appIconImageName = "gbIcon",
};

k_app gameboyAdvance = {
    .name = "GameBoy Advance",
    .entryPoint = &GameboyAdvanceEntryPoint,
    .appIconImageName = "gbaIcon",
    .requestedStorageBytes = 0 //8KB of storage for Gameboy
};

k_app mainMenu = {
    .name = "Main menu",
    .entryPoint = &MainMenuEntryPoint,
    .appIconImageName = "homeIcon",
    .requestedStorageBytes = 0
};

k_app testing = {
    .name = "Test mode",
    .entryPoint = &TestModeEntryPoint,
    .appIconImageName = "verificationIcon",
    .requestedStorageBytes = 8 * 1000 //8KB of storage mainly for testing the internal falsh
};

k_app settings = {
    .name = "Settings",
    .entryPoint = &SettingsEntryPoint,
    .appIconImageName = "settingsIcon",
    .requestedStorageBytes = 4 * 1000 //4KB of storage should be more than enough
};

k_app downloadMode = {
    .name = "Download mode",
    .entryPoint = &DownloadModeEntryPoint,
    //.appIcon = DownloadMode_Icon,
    .appIconImageName = "downloadIcon"
};

k_app turnOff = {
    .name = "Turn off",
    .entryPoint = &TurnOffEntryPoint,
    .appIconImageName = "turnOffIcon",
};

void k_registerApps(){    
    k_registerApp(&mainMenu);  
    k_registerApp(&doom);    
    k_registerApp(&gameboyColor);   
    k_registerApp(&gameboyAdvance);   
    k_registerApp(&testing);    
    k_registerApp(&settings);
    k_registerApp(&downloadMode);
    //k_registerApp(&turnOff);
}


void k_InitializeSettings(){

    //Actions, for internal use only. Functions defined in the settings app
    k_AddSettingIfNotExistent("/Actions/Reset settings",Action, 0, 0);
    k_AddSettingIfNotExistent("/Actions/Reset storage (inc. settings)",Action, 0, 0);
    k_AddSettingIfNotExistent("/Actions/Invalidate checkpoints",Action, 0, 0);
    //End of actions

    bool defaultCheckpointCreationEnabled = true;
    k_AddSettingIfNotExistent("/Intermittency/Checkpoint creation enabled",Bool, &defaultCheckpointCreationEnabled, sizeof(bool));
    bool defaultCheckpointRecoveryEnabled = true;
    k_AddSettingIfNotExistent("/Intermittency/Checkpoint recovery enabled",Bool, &defaultCheckpointRecoveryEnabled, sizeof(bool));
    uint32_t defaultCheckpointFreq = 1000;
    k_AddSettingIfNotExistent("/Intermittency/Checkpoint interval (ms)",Uint32, &defaultCheckpointFreq, sizeof(uint32_t));


    bool defaultCheckpointOnlyWhenActive = true;
    k_AddSettingIfNotExistent("/Intermittency/Only checkpoint while active",Bool, &defaultCheckpointOnlyWhenActive, sizeof(bool));
    uint32_t defaultActiveTimeout = 5000;
    k_AddSettingIfNotExistent("/Intermittency/Active timeout (ms)",Uint32, &defaultActiveTimeout, sizeof(uint32_t));
   
    uint32_t defaultScreenBrightNess = 3;
    k_AddSettingIfNotExistent("/Energy/Screen brightness",Uint32, &defaultScreenBrightNess, sizeof(uint32_t));
    bool automaticBrightness = false;
    k_AddSettingIfNotExistent("/Energy/Automatic brightness",Bool, &automaticBrightness, sizeof(bool));    
    bool defaultInfiniteBattery = false;
    k_AddSettingIfNotExistent("/Energy/Infinite battery",Bool, &defaultInfiniteBattery, sizeof(bool));
    bool highPerformanceMode = true;
    k_AddSettingIfNotExistent("/Energy/Fast CPU",Bool, &highPerformanceMode, sizeof(bool));
    bool highLCDrate = false;
    k_AddSettingIfNotExistent("/Energy/Fast LCD refresh",Bool, &highLCDrate, sizeof(bool));
    uint32_t defaultMaxFPS = 20;
    k_AddSettingIfNotExistent("/Energy/Target refresh rate (Hz)",Uint32, &defaultMaxFPS, sizeof(uint32_t));

    bool preventDefaultInputWhenCrankEnabled = true;
    k_AddSettingIfNotExistent("/Energy/No input when crank enabled",Bool, &preventDefaultInputWhenCrankEnabled, sizeof(bool));

    bool defaultShowFPS = false;
    k_AddSettingIfNotExistent("/Debugging/Overlay/Show FPS",Bool, &defaultShowFPS, sizeof(bool));
    bool defaultShowHeapChanges = false;
    k_AddSettingIfNotExistent("/Debugging/Overlay/Show heap modifications",Bool, &defaultShowHeapChanges, sizeof(bool));
    bool defaultSendScreenOverRTT = false;
    k_AddSettingIfNotExistent("/Debugging/Send screen over RTT",Bool, &defaultSendScreenOverRTT, sizeof(bool));
    bool defaultShowBattery = true;
    k_AddSettingIfNotExistent("/Debugging/Overlay/Show battery %%",Bool, &defaultShowBattery, sizeof(bool));
    bool defaultShowCrankStatus = true;
    k_AddSettingIfNotExistent("/Debugging/Overlay/Show crank status",Bool, &defaultShowCrankStatus, sizeof(bool));


    
    bool defaultDOOMstartDemo = true;
    k_AddSettingIfNotExistent("/DOOM/Enable DEMO mode",Bool, &defaultShowHeapChanges, sizeof(bool));
    uint32_t defaultDOOMDEMOselection = 3;
    k_AddSettingIfNotExistent("/DOOM/DEMO mode selection",Uint32,  &defaultDOOMDEMOselection, sizeof(uint32_t));
    bool defaultDOOMforceEnterLevel = true;
    k_AddSettingIfNotExistent("/DOOM/Force enter level",Bool, &defaultDOOMforceEnterLevel, sizeof(bool));
    uint32_t defaultDOOMEpisode = 9;
    k_AddSettingIfNotExistent("/DOOM/Enter level episode",Uint32,  &defaultDOOMEpisode, sizeof(uint32_t));
    uint32_t defaultDOOMgamma = 2;
    k_AddSettingIfNotExistent("/DOOM/Brightness",Uint32,  &defaultDOOMgamma, sizeof(uint32_t));
    bool doomUseCrank = true;
    k_AddSettingIfNotExistent("/DOOM/Use crank",Bool,  &doomUseCrank, sizeof(bool));

    bool defaultGBCcrankAsAB = true;
    k_AddSettingIfNotExistent("/GBC/Use crank as A-B input",Bool, &defaultGBCcrankAsAB, sizeof(bool));

    bool defaultSlowDownWhileCranking = true;
    k_AddSettingIfNotExistent("/GBC/Slow down time when cranking",Bool, &defaultSlowDownWhileCranking, sizeof(bool));


    //Actions, for internal use only. Functions defined in the settings app
    k_AddSettingIfNotExistent("/Experiments/Start experiment",Action, 0, 0);
    k_AddSettingIfNotExistent("/Experiments/Stop experiment",Action, 0, 0);
    uint32_t defaultExperimentID = 0;
    k_AddSettingIfNotExistent("/Experiments/Experiment ID",Uint32, &defaultExperimentID, sizeof(defaultExperimentID));
    //End of actions

    bool pauseEnabled = true;
    k_AddSettingIfNotExistent("/Experiments/Pause enabled",Bool,  &pauseEnabled, sizeof(bool));


    //Actions, for internal use only. Functions defined in the settings app
    k_AddSettingIfNotExistent("/Experiments/Presets/A (%%, crank & battery)",Action, 0, 0);
    k_AddSettingIfNotExistent("/Experiments/Presets/B (Nothing)",Action, 0, 0);
    k_AddSettingIfNotExistent("/Experiments/Presets/C (%%)",Action, 0, 0);
    k_AddSettingIfNotExistent("/Experiments/Presets/D (%% & crank)",Action, 0, 0);
    k_AddSettingIfNotExistent("/Experiments/Presets/E (No pause & %%)",Action, 0, 0);
    //End of actions
}