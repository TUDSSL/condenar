#include "Settings.h"
#include "Kernel.h"
#include "TimeKeeper.h"
#include "Kernel_Draw.h"
#include "stdlib.h"
#include "Helpers/Misc.h"
#include "string.h"
#include "SettingsHandler.h"
#include "AppStorage.h"

#define MAX_SETTINGS_DEPTH 25
#define SETTINGS_LINE_SEPARATION 20
#define MAX_ANTI_REPEAT_FOLDER 10

CHECKPOINT_EXCLUDE_DATA uint8_t folder_icon[] = {
	0xff, 0xff, 0x80, 0xff, 0xff, 0x80, 0x00, 0xff, 0x80, 0x00, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 
	0xff, 0xff, 0x80
};
CHECKPOINT_EXCLUDE_DATA uint8_t back_icon[] = {
	0xff, 0xff, 0x80, 0xff, 0xff, 0x80, 0xe7, 0xff, 0x80, 0xc7, 0xff, 0x80, 0x80, 0x0f, 0x80, 0x80, 
	0x03, 0x80, 0xc7, 0xe1, 0x80, 0xe7, 0xf9, 0x80, 0xff, 0xfc, 0x80, 0xff, 0xfc, 0x80, 0xff, 0xfc, 
	0x80, 0xff, 0xf8, 0x80, 0xff, 0xf1, 0x80, 0xfe, 0x03, 0x80, 0xff, 0x07, 0x80, 0xff, 0xff, 0x80, 
	0xff, 0xff, 0x80
};

CHECKPOINT_EXCLUDE_DATA uint8_t action_icon[] = {
	0xff, 0xbf, 0x80, 0xff, 0xbf, 0x80, 0xff, 0x3f, 0x80, 0xff, 0x3f, 0x80, 0xfe, 0x3f, 0x80, 0xfc, 
	0x7f, 0x80, 0xfc, 0x7f, 0x80, 0xf8, 0x07, 0x80, 0xf0, 0x07, 0x80, 0xf0, 0x0f, 0x80, 0xff, 0x1f, 
	0x80, 0xff, 0x1f, 0x80, 0xfe, 0x3f, 0x80, 0xfe, 0x7f, 0x80, 0xfe, 0x7f, 0x80, 0xfe, 0xff, 0x80, 
	0xfe, 0xff, 0x80
};

CHECKPOINT_EXCLUDE_DATA uint8_t exit_icon[] = {
	0xff, 0xff, 0xc0, 0x80, 0x1f, 0xc0, 0x80, 0x0f, 0xc0, 0x9f, 0xcf, 0xc0, 0x9f, 0xff, 0xc0, 0x9f, 
	0xf3, 0xc0, 0x9f, 0xf9, 0xc0, 0x9f, 0xfc, 0xc0, 0x90, 0x00, 0x40, 0x90, 0x00, 0x40, 0x9f, 0xfc, 
	0xc0, 0x9f, 0xf9, 0xc0, 0x9f, 0xf3, 0xc0, 0x9f, 0xff, 0xc0, 0x9f, 0xcf, 0xc0, 0x80, 0x0f, 0xc0, 
	0x80, 0x1f, 0xc0, 0xff, 0xff, 0xc0
};

void Settings_Update();
void Settings_Render();
void ExecuteCustomAction(char* actionPath);
void changeSetting(char* settingsPath, SettingsType settingsType, int change);

int selectedOptionVertical = 0;
uint32_t lastMillisMenuChange = 0;

int lastMaxCurrentSettingsPageLength = 0;

char currentlyHoveringSettingsPath[SETTINGS_MAX_NAME_LENGHT] = "";
SettingsType currentlyHoveringSettingType;
bool isCurrentlyHoveringSettingAFolder = false;
char currentSettingsPath[SETTINGS_MAX_NAME_LENGHT] = "/";

int selectedIndexStack[MAX_SETTINGS_DEPTH];
int currentSelectedIndexStackIndex = 0;

bool waitingForButtonsUnpresssed=false;

void SettingsEntryPoint(){
	waitingForButtonsUnpresssed = true;
	selectedOptionVertical = 0;
	strcpy(currentSettingsPath,"/");

    while(!k_getShouldCurrentAppExit()){
        Settings_Update();
        Settings_Render();
    }
}


void Settings_Update(){

	k_input_state state;
	k_GetInput(&state, true);

	if(state.joystickY > 0.7){
		if(millis()-lastMillisMenuChange>350){
			if(selectedOptionVertical ==0){
				selectedOptionVertical = lastMaxCurrentSettingsPageLength;
			}else{
				selectedOptionVertical--;
			}
			lastMillisMenuChange = millis();
		}		
	}else if(state.joystickY < -0.7){
		if(millis()-lastMillisMenuChange>350){
			selectedOptionVertical++;
			lastMillisMenuChange = millis();
		}
	}else{ //Joystick not pressed, if enough time has passed, get rid of the lastMillis for quick presses
		if(millis()-lastMillisMenuChange>100){
			lastMillisMenuChange = 0;
		}
	}
	
	if(waitingForButtonsUnpresssed){
		if(state.buttonA || state.buttonB || state.joystickX>0 || state.joystickX<0){
			return; //We are waiting for the buttons to get unpressed
		}else{
			waitingForButtonsUnpresssed=false;
		}
	}

	if(state.buttonA ||state.joystickX>0.75f || state.joystickX<-0.75f){
		if(millis()- lastMillisMenuChange>250 && strlen(currentSettingsPath)>0){
			lastMillisMenuChange=millis();
			waitingForButtonsUnpresssed=true;
			if(isCurrentlyHoveringSettingAFolder){
				if(strcmp(currentlyHoveringSettingsPath,"/Exit")==0){ //just exit, custom thing
					k_setShouldCurrentAppExit(true);
					return;
				}
				strcpy(currentSettingsPath, currentlyHoveringSettingsPath);
				currentlyHoveringSettingsPath[0] = 0; //Empty the string
				if(currentSelectedIndexStackIndex<MAX_SETTINGS_DEPTH){
					selectedIndexStack[currentSelectedIndexStackIndex] = selectedOptionVertical;
					currentSelectedIndexStackIndex++;
				}
				selectedOptionVertical = 0;
			}else if(currentlyHoveringSettingType==Action || currentlyHoveringSettingType == Bool){
				changeSetting(currentlyHoveringSettingsPath, currentlyHoveringSettingType, 1);
			}else if(currentlyHoveringSettingType==Int32|| currentlyHoveringSettingType == Uint32){
				changeSetting(currentlyHoveringSettingsPath, currentlyHoveringSettingType, (state.buttonA || state.joystickX>0)? 1:-1);
			}
		}
	}else if(state.buttonB){

		if(millis()- lastMillisMenuChange>250){
			if(strcmp(currentSettingsPath,"/")==0){ //just exit, custom thing
				k_setShouldCurrentAppExit(true);
				return;
			}else{  //Not in root (/), checked in the strcmp
				lastMillisMenuChange=millis();
				waitingForButtonsUnpresssed=true;
				char* lastSlash = strrchr(currentSettingsPath,'/'); 
				*(lastSlash) = '\0';	 //Remove the last subfolder (i.e. go back one)
				if(strlen(currentSettingsPath)==0){
					currentSettingsPath[0]='/';
					currentSettingsPath[1]=0;
				}
				if(currentSelectedIndexStackIndex>0){
					currentSelectedIndexStackIndex--;
					selectedOptionVertical=selectedIndexStack[currentSelectedIndexStackIndex];
				}			
			}			
		}
	}else if(state.joystickX > 0.75f){
		if(!isCurrentlyHoveringSettingAFolder){
			if(millis()- lastMillisMenuChange>250){ //Not in root (/)
				lastMillisMenuChange=millis();
				waitingForButtonsUnpresssed=true;
				if(strlen(currentlyHoveringSettingsPath)>1){ //Not in root
					changeSetting(currentlyHoveringSettingsPath,currentlyHoveringSettingType, 1);
				}			
			}
		}		
	}else if(state.joystickX < -0.75f){
		if(!isCurrentlyHoveringSettingAFolder){
			if(millis()- lastMillisMenuChange>250){ //Not in root (/)
				lastMillisMenuChange=millis();
				waitingForButtonsUnpresssed=true;
				if(strlen(currentlyHoveringSettingsPath)>1){ //Not in root
					changeSetting(currentlyHoveringSettingsPath,currentlyHoveringSettingType, -1);
				}			
			}
		}			
	}    
}




void Settings_Render(){
    k_BeginScreenUpdate();
    k_draw_Clear(K_COLOR_WHITE);

	k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,20, 1, currentSettingsPath);

	char alreadyWrittenFolders[MAX_ANTI_REPEAT_FOLDER][SETTINGS_MAX_NAME_LENGHT];
	int alreadyWrittenFoldersNextIndex=0;

	currentlyHoveringSettingsPath[0] = 0; //Empty the string

	void* token = NULL;
	char nextName[SETTINGS_MAX_NAME_LENGHT];
	char nextNameToDisplay[SETTINGS_MAX_NAME_LENGHT];
	SettingsType nextType;
	int settingsDrawnCount = 0;
	while((token = k_EnumerateNextSettingEntry(currentSettingsPath,token,nextName,&nextType))!=NULL){

		strcpy(nextNameToDisplay,nextName);

		bool isInDeeperFolder = (strrchr(nextName,'/')-nextName) > strlen(currentSettingsPath); //If there are '/' after the current folder path, it is in a subfolder

		int yCoord = 50 + (SETTINGS_LINE_SEPARATION*settingsDrawnCount);

		if(isInDeeperFolder){

			//Get the path for the subfolder instead of the item
			char* nextSlash = strchr(nextName + strlen(currentSettingsPath)+1,'/');
			*nextSlash = '\0';			

			//If we have already drawn this folder, we want to skip it
			bool alreadyWritten = false;
			for(int i=0;i<alreadyWrittenFoldersNextIndex;i++){				
				if(strcmp(nextName,alreadyWrittenFolders[i])==0){
					goto skipItem; //We don't want to draw this one, already drawn
				}
			}	

			//Not previously drawn
			//Add it to the already drawn list
			if(alreadyWrittenFoldersNextIndex<MAX_ANTI_REPEAT_FOLDER){
				strcpy(alreadyWrittenFolders[alreadyWrittenFoldersNextIndex], nextName);
				alreadyWrittenFoldersNextIndex++;
			}
			//Draw item as usual
			char* finalFolderOrSettingName = strrchr(nextName,'/'); 
			strcpy(nextNameToDisplay, finalFolderOrSettingName);	
		}else{
			//Is just an item, draw as usual without the folder path
			char* finalFolderOrSettingName = strrchr(nextNameToDisplay,'/'); 
			strcpy(nextNameToDisplay, finalFolderOrSettingName);	
		}


		
		
		if(selectedOptionVertical == settingsDrawnCount)
		{
			k_draw_DrawTextLeftAligned(15, yCoord,1,">");	
			strcpy(currentlyHoveringSettingsPath,nextName);
			currentlyHoveringSettingType = nextType;
			isCurrentlyHoveringSettingAFolder = isInDeeperFolder;
		}
		 	
		k_draw_DrawTextLeftAligned(28, yCoord,1,nextNameToDisplay + 1);

		if(!isInDeeperFolder){ //Draw an acutal setting
			switch(nextType){
				case Bool:
					k_draw_DrawTextCentered(SCREEN_WIDTH_REAL - 40,yCoord, 1, k_GetSettingBool(nextName, false)?"true":"false");
				break;
				case Uint32:
					k_draw_DrawTextCentered(SCREEN_WIDTH_REAL - 40,yCoord, 1, "%d",k_GetSettingUInt32(nextName, 0));
				break;
				case Int32:
					k_draw_DrawTextCentered(SCREEN_WIDTH_REAL - 40,yCoord, 1, "%d",k_GetSettingInt32(nextName, 0));
				break;
				case Action:
					k_draw_DrawImage(SCREEN_WIDTH_REAL - 40 + 8, yCoord - 4, 17,17,1,action_icon);
				break;
				default:
				break;
			}
		}else{ //Draw folder icon
			k_draw_DrawImage(SCREEN_WIDTH_REAL - 40 + 8, yCoord - 4, 17,17,1,folder_icon);
		}

		settingsDrawnCount++;

skipItem:;
	}

	lastMaxCurrentSettingsPageLength = settingsDrawnCount;

	if(selectedOptionVertical>settingsDrawnCount) selectedOptionVertical = 0;

	if(selectedOptionVertical == settingsDrawnCount)
	{
		k_draw_DrawTextLeftAligned(15, SCREEN_HEIGHT_REAL - 35,1,">");	
		isCurrentlyHoveringSettingAFolder = true;

		if(strlen(currentSettingsPath)>1){
			strcpy(currentlyHoveringSettingsPath, currentSettingsPath);
			char* lastSlash = strrchr(currentlyHoveringSettingsPath,'/'); 
			*(lastSlash) = '\0';	 //Remove the last subfolder (i.e. go back one)
			if(strlen(currentlyHoveringSettingsPath)==0){
				currentlyHoveringSettingsPath[0]='/';
				currentlyHoveringSettingsPath[1]=0;
			}
		}else{
			strcpy(currentlyHoveringSettingsPath,"/Exit");
		}

	}

	if(strlen(currentSettingsPath)>1)
	{
		k_draw_DrawTextLeftAligned(28, SCREEN_HEIGHT_REAL - 35,1,"Go back");
	 	k_draw_DrawImage(SCREEN_WIDTH_REAL - 40 + 8, SCREEN_HEIGHT_REAL - 35 - 4, 17,17,1,back_icon);
	}else{
		k_draw_DrawTextLeftAligned(28, SCREEN_HEIGHT_REAL - 35,1,"Exit");
	 	k_draw_DrawImage(SCREEN_WIDTH_REAL - 40 + 8, SCREEN_HEIGHT_REAL - 35 - 4, 18,18,1,exit_icon);
	}

    k_EndScreenUpdate(true,true);
}


void ExecuteCustomAction(char* actionPath){
	if(strcmp(actionPath,"/Actions/Reset settings")==0){
		k_ResetAllSetings();
		k_InitializeSettings();
	}else if(strcmp(actionPath,"/Actions/Reset storage (inc. settings)")==0){
		k_AppStorageReset();
		k_ForceRestart();
	}else if(strcmp(actionPath,"/Actions/Invalidate checkpoints")==0){
		
	}else if(strcmp(actionPath,"/Experiments/Start experiment")==0){
		UART_LOG_StartExperiment(k_GetSettingUInt32("/Experiments/Experiment ID", 0));
	}else if(strcmp(actionPath,"/Experiments/Stop experiment")==0){
		UART_LOG_EndExperiment();
	}else if(strcmp(actionPath,"/Experiments/Presets/A (%%, crank & battery)")==0){
		k_SetSettingBool("/Energy/Infinite battery",true);		
		k_SetSettingBool("/DOOM/Use crank",true);
		k_SetSettingBool("/GBC/Use crank as A-B input",true);
		k_SetSettingBool("/GBC/Slow down time when cranking",true);
		k_SetSettingBool("/Debugging/Overlay/Show battery %%",true);
		k_SetSettingBool("/Experiments/Pause enabled",true);	
	}else if(strcmp(actionPath,"/Experiments/Presets/B (Nothing)")==0){
		k_SetSettingBool("/Energy/Infinite battery",false);		
		k_SetSettingBool("/DOOM/Use crank",false);
		k_SetSettingBool("/GBC/Use crank as A-B input",false);
		k_SetSettingBool("/GBC/Slow down time when cranking",false);
		k_SetSettingBool("/Debugging/Overlay/Show battery %%",false);
		k_SetSettingBool("/Experiments/Pause enabled",true);	
	}else if(strcmp(actionPath,"/Experiments/Presets/C (%%)")==0){
		k_SetSettingBool("/Energy/Infinite battery",false);		
		k_SetSettingBool("/DOOM/Use crank",false);
		k_SetSettingBool("/GBC/Use crank as A-B input",false);
		k_SetSettingBool("/GBC/Slow down time when cranking",false);
		k_SetSettingBool("/Debugging/Overlay/Show battery %%",true);
		k_SetSettingBool("/Experiments/Pause enabled",true);	
	}else if(strcmp(actionPath,"/Experiments/Presets/D (%% & crank)")==0){
		k_SetSettingBool("/Energy/Infinite battery",false);		
		k_SetSettingBool("/DOOM/Use crank",true);
		k_SetSettingBool("/GBC/Use crank as A-B input",true);
		k_SetSettingBool("/GBC/Slow down time when cranking",true);
		k_SetSettingBool("/Debugging/Overlay/Show battery %%",true);
		k_SetSettingBool("/Experiments/Pause enabled",true);	
	}else if(strcmp(actionPath,"/Experiments/Presets/E (No pause & %%)")==0){
		k_SetSettingBool("/Energy/Infinite battery",false);		
		k_SetSettingBool("/DOOM/Use crank",true);
		k_SetSettingBool("/GBC/Use crank as A-B input",true);
		k_SetSettingBool("/GBC/Slow down time when cranking",true);
		k_SetSettingBool("/Debugging/Overlay/Show battery %%",true);
		k_SetSettingBool("/Experiments/Pause enabled",false);				
	}

	//Draw success message and wait 1 second
	k_BeginScreenUpdate();
    k_draw_Clear(K_COLOR_WHITE);
	k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,SCREEN_HEIGHT_REAL/2 - 4, 1, "Action complete!");
	k_EndScreenUpdate(true,true);
	DelayMS(1000);
}


void changeSetting(char* settingsPath, SettingsType settingsType, int change){
	switch (settingsType)
	{
	case Bool:
		k_SetSettingBool(settingsPath,!k_GetSettingBool(settingsPath,false));
		break;
	case Int32:
		k_SetSettingInt32(settingsPath,k_GetSettingInt32(settingsPath,0)+change);
		break;
	case Uint32:
		k_SetSettingUInt32(settingsPath,k_GetSettingUInt32(settingsPath,0)+change);
		break;		
	case Action:
		ExecuteCustomAction(settingsPath);
	default:
		break;
	}
}


CHECKPOINT_EXCLUDE_DATA uint8_t Settings_Icon[] = {
	0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xc0, 0xff, 0xff, 
	0xc0, 0x00, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 
	0xff, 0xff, 0xc0, 0xff, 0xff, 0x80, 0x00, 0x7f, 0xff, 0xc0, 0xff, 0xff, 0x80, 0x00, 0x7f, 0xff, 
	0xc0, 0xfe, 0x7f, 0x80, 0x00, 0x7f, 0x9f, 0xc0, 0xfc, 0x1f, 0x00, 0x00, 0x3e, 0x0f, 0xc0, 0xf8, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0xf0, 0x00, 0x00, 
	0x00, 0x00, 0x03, 0xc0, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0xe0, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x40, 0x80, 0x00, 
	0x0f, 0xfc, 0x00, 0x00, 0x40, 0x80, 0x00, 0x1f, 0xfe, 0x00, 0x00, 0x40, 0xe0, 0x00, 0x3f, 0xff, 
	0x00, 0x01, 0xc0, 0xf0, 0x00, 0x7f, 0xff, 0x80, 0x03, 0xc0, 0xf8, 0x00, 0x7f, 0xff, 0x80, 0x07, 
	0xc0, 0xfc, 0x00, 0x7f, 0xff, 0x80, 0x0f, 0xc0, 0xfc, 0x00, 0x7f, 0xff, 0x80, 0x0f, 0xc0, 0xfc, 
	0x00, 0x7f, 0xff, 0x80, 0x0f, 0xc0, 0xfc, 0x00, 0x7f, 0xff, 0x80, 0x0f, 0xc0, 0xf8, 0x00, 0x7f, 
	0xff, 0x80, 0x07, 0xc0, 0xf0, 0x00, 0x7f, 0xff, 0x80, 0x03, 0xc0, 0xe0, 0x00, 0x3f, 0xff, 0x00, 
	0x01, 0xc0, 0x80, 0x00, 0x1f, 0xfe, 0x00, 0x00, 0x40, 0x80, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x40, 
	0x80, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xe0, 0x00, 0x00, 0x00, 
	0x00, 0x01, 0xc0, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x03, 
	0xc0, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0xfc, 
	0x1f, 0x00, 0x00, 0x3e, 0x0f, 0xc0, 0xfe, 0x7f, 0x80, 0x00, 0x7f, 0x9f, 0xc0, 0xff, 0xff, 0x80, 
	0x00, 0x7f, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 0x7f, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 0xff, 
	0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xc0, 
	0xff, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xe0, 0x01, 0xff, 0xff, 0xc0
};

