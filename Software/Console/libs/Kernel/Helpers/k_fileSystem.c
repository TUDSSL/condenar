#include "Kernel.h"
#include "Kernel_Draw.h"
#include "FileSystem.h"
#include "Helpers/Flash.h"
#include "Misc.h"
#include "am_util_stdio.h"

typedef struct __attribute__ ((packed)) FileEntry{
    uint8_t fileType; 
    uint8_t magicNumber; //0x69   
    uint16_t param1;
    uint16_t param2;
    uint8_t name[20];  //Includes null terminator
    uint32_t dataSize;
    //uint8_t* data; Data comes here
} FileEntry;

//Returns whether the file exists
bool GetFileByName(const char* name, uint8_t** addressOut, uint32_t* sizeOut, uint16_t* param1Out, uint16_t* param2Out, FileSystemTypes* fileTypeOut){
    FileEntry* currentMemoryAddress = (FileEntry*)EXT_FLASH_FILE_STORAGE_ADDRESS;

    while(currentMemoryAddress < (EXT_FLASH_MAX_ADRESSS - sizeof(FileEntry))){
        if(currentMemoryAddress->magicNumber == 0x69){ //Used block
            if(strcmp(currentMemoryAddress->name, name) == 0){
                if(addressOut!=NULL) *addressOut = (uint8_t*)currentMemoryAddress + sizeof(FileEntry);
                if(sizeOut!=NULL) *sizeOut = currentMemoryAddress->dataSize;
                if(param1Out!=NULL) *param1Out = currentMemoryAddress->param1;
                if(param2Out!=NULL) *param2Out = currentMemoryAddress->param2;
                if(fileTypeOut!=NULL) *fileTypeOut = (FileSystemTypes)currentMemoryAddress->fileType;
                return true;
            }
            currentMemoryAddress = (FileEntry*)((uint8_t*)currentMemoryAddress + sizeof(FileEntry) + currentMemoryAddress->dataSize);
        }else{
            return false; //Block not used, we are at the end of the block list
        }        
    }
    return false;
}

k_image GetImageByName(const char* name){

    k_image image = {0};
    image.ready = false; //Not found or invalid by default

    uint8_t* address;
    uint32_t size;
    uint16_t param1;
    uint16_t param2;
    FileSystemTypes fileType;
    if(GetFileByName(name, &address, &size, &param1, &param2, &fileType)){
        if(param1 != 0 && param2 != 0){
            image.ready = true;
            image.dataPtr = address;
            image.width = param1;
            image.height = param2;

            switch (fileType)
            {
            case RGBA2221_IMAGE:
                image.format = K_IMAGE_FORMAT_RGBA2221;
                break;            
            default:
                image.ready = false;
                break;
            }
        }
    }

    return image;    
}

char* GetNameForFileType(FileSystemTypes type){
    switch (type)
    {   
        case GB:
            return "GB";
        case GBC:
            return "GBC";
        case GBA_BIOS:
            return "GBA BIOS";
        case GBA:
            return "GBA";
        case RGBA2221_IMAGE:
            return "RGBA2221 Image";
    default:
        return "N/A";
    }
}

//Returns false if the user wants to exit. Otherwise, returns true and sets the addressOut and sizeOut to the file selected
bool ShowFileSelection(uint8_t fileType, uint8_t** addressOut, uint32_t* sizeOut, uint16_t* param1Out, uint16_t* param2Out){
    int currentCursorPosition = 0;
    int currentPageIndex = 0;

    int itemsPerPage = 5;

    k_app* currentApp = k_getCurrentApp();


    bool previousStateEmpty = false; //Whether there was any input in the previous frame

    volatile uint32_t startMillis = k_GetTimeMS();

    k_input_state state;
    do{        
        k_GetInput(&state,true);

        k_BeginScreenUpdate();
        k_draw_Clear(K_COLOR_WHITE);

        k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,90,1,"Select a file to load:");
        k_draw_DrawTextLeftAligned(10,SCREEN_HEIGHT_REAL - 15,1,"Press A to select");
        k_draw_DrawTextLeftAligned(190,SCREEN_HEIGHT_REAL - 15,1,"Press B to exit");

        k_draw_DrawImageGeneric(&currentApp->appIconImage, SCREEN_WIDTH_REAL/2 - currentApp->appIconImage.width/2,45 - currentApp->appIconImage.height/2,1);
        k_draw_DrawRectangle(15, 109,SCREEN_WIDTH_REAL-30,itemsPerPage*16 + 4 + 18, K_COLOR_BLACK);
        k_draw_DrawRectangle(15, 193,SCREEN_WIDTH_REAL-30,18, K_COLOR_BLACK);

        FileEntry* currentMemoryAddress = (FileEntry*)EXT_FLASH_FILE_STORAGE_ADDRESS;

        int currentLine = 0;
        int currentFileIndex = 0;
        while(currentMemoryAddress < (EXT_FLASH_MAX_ADRESSS - sizeof(FileEntry))){
            if(currentMemoryAddress->magicNumber == 0x69){ //Used block
                if((currentMemoryAddress->fileType & fileType) > 0){
                    if(currentFileIndex >= currentPageIndex*itemsPerPage && currentLine < itemsPerPage){
                        char buffer[32] = { 0 };
                        am_util_stdio_snprintf(buffer,sizeof(buffer)-1 , "[%s] %s" ,GetNameForFileType(currentMemoryAddress->fileType),currentMemoryAddress->name);
                        if(currentFileIndex == currentCursorPosition){
                            k_draw_DrawTextLeftAligned(20,115+currentLine*16,1,">");
                            k_draw_DrawTextLeftAligned(30,115+currentLine*16,1,buffer);
                        }else{
                            k_draw_DrawTextLeftAligned(30,115+currentLine*16,1,buffer);
                        }
                        currentLine++;
                    }
                    /*char buffer[32] = { 0 };
                    am_util_stdio_snprintf(buffer,sizeof(buffer)-1 , "[%s] %s" ,GetNameForFileType(currentMemoryAddress->fileType),currentMemoryAddress->name);
                    if(currentFileIndex == currentCursorPosition){
                        k_draw_DrawTextLeftAligned(20,120+currentLine*16,1,">");
                        k_draw_DrawTextLeftAligned(30,120+currentLine*16,1,buffer);
                    }else{
                        k_draw_DrawTextLeftAligned(30,120+currentLine*16,1,buffer);
                    }*/
                    currentFileIndex++;
                }
                currentMemoryAddress = (FileEntry*)((uint8_t*)currentMemoryAddress + sizeof(FileEntry) + currentMemoryAddress->dataSize);
            }else{
                break; //Block not used, we are at the end of the block list
            }        
        }

        k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,SCREEN_HEIGHT_REAL - 42,1,"Page %d/%d",currentPageIndex+1, (currentFileIndex/itemsPerPage)+1);

        k_EndScreenUpdate(true, true);

        if(k_GetTimeMS()-startMillis < 250) previousStateEmpty = false; //Prevent accidental selection on enter

        if(state.buttonA && previousStateEmpty){
            currentFileIndex = 0;
            currentMemoryAddress = (FileEntry*)EXT_FLASH_FILE_STORAGE_ADDRESS;
            while(currentMemoryAddress < (EXT_FLASH_MAX_ADRESSS - sizeof(FileEntry))){
                if(currentMemoryAddress->magicNumber == 0x69){ //Used block
                    if((currentMemoryAddress->fileType & fileType) > 0){
                        if(currentFileIndex == currentCursorPosition){
                            if(addressOut!=NULL) *addressOut = (uint8_t*)currentMemoryAddress + sizeof(FileEntry);
                            if(sizeOut!=NULL) *sizeOut = currentMemoryAddress->dataSize;
                            if(param1Out!=NULL) *param1Out = currentMemoryAddress->param1;
                            if(param2Out!=NULL) *param2Out = currentMemoryAddress->param2;
                            return true;
                        }
                        currentFileIndex++;
                    }
                    currentMemoryAddress = (FileEntry*)((uint8_t*)currentMemoryAddress + sizeof(FileEntry) + currentMemoryAddress->dataSize);
                }else{
                    break; //Block not used, we are at the end of the block list
                }        
            }
            LOG_I("File not found");
        }else if(state.buttonB  && previousStateEmpty){
            return false;
        }else if(state.joystickY > 0  && previousStateEmpty){
            previousStateEmpty = false;
            currentCursorPosition--;
            if(currentCursorPosition < 0){
                currentCursorPosition = currentFileIndex - 1;
                currentPageIndex = (currentFileIndex-1)/itemsPerPage;
            }else{
                if(currentCursorPosition < currentPageIndex*itemsPerPage){
                    currentPageIndex--;
                }
            }

        } else if(state.joystickY < 0  && previousStateEmpty){
            previousStateEmpty = false;
            currentCursorPosition++;
            if(currentCursorPosition >= currentFileIndex){
                currentCursorPosition = 0;
                currentPageIndex = 0;
            }else{
                if(currentCursorPosition >= (currentPageIndex+1)*itemsPerPage){
                    currentPageIndex++;
                }
            }

        }else{
            if(state.joystickY==0) previousStateEmpty = true;
        }
    }while(!(state.triggerLeft && state.triggerRight && state.buttonA));

    return false;
}