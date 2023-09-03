#include "SettingsHandler.h"
#include "Kernel.h"
#include "string.h"
#include "Helpers/Misc.h"
#include "MRAM.h"

k_settingsItem* k_GetROSettingsBlock(char* path, SettingsType type){
    uint32_t settingsSize = 0;
    k_settingsItem* currentItem = (k_settingsItem*)k_GetStorageReadOnlyAddressForApp(k_getAppByName("Settings"), &settingsSize);
    if(currentItem==NULL) return false;

    uint32_t howManySettingsFit = settingsSize / sizeof(k_settingsItem);
   
    uint32_t currentCount=0;
    while(currentCount < howManySettingsFit){
        if(currentItem->cookie != SETTINGS_COOKIE) break;

        if(strncmp(path, currentItem->name, SETTINGS_MAX_NAME_LENGHT)==0 && currentItem->dataType==type){           
            return currentItem;
        }

        currentCount++;
        currentItem++;
    }

    //Couldn't find that setting
    return NULL;
}

void k_GetMaximumSettingLength(uint32_t* maxNameLengthOut, uint32_t* maxContentSizeOut){
    *maxNameLengthOut = SETTINGS_MAX_NAME_LENGHT;
    *maxContentSizeOut = SETTINGS_MAX_DATA_LENGHT;
}

bool k_GetSettingGeneric(char* path, SettingsType type, void* dataOut, uint32_t bytesToCopy){

    k_settingsItem* item = k_GetROSettingsBlock(path,type);
    if(item==NULL) return false;

    if(bytesToCopy>SETTINGS_MAX_DATA_LENGHT) {
        LOG_I("Setting requested with a size too large! %s", path);
        return false;
    }

    memcpy(dataOut, item->data, bytesToCopy);

    return true;
}

bool k_SetSettingGeneric(char* path,SettingsType type, void* dataIn, uint32_t bytesToCopy){

    k_settingsItem* item = k_GetROSettingsBlock(path, type);


    if(strlen(path) <= 1){ //Empty path
        return false;
    }

    if(bytesToCopy>SETTINGS_MAX_DATA_LENGHT) {
        LOG_I("Setting requested with a size too large! %s", path);
        return false;
    }

    if(item==NULL) {
        return k_AddSettingIfNotExistent(path, type, dataIn, bytesToCopy);
    }else{
        k_copyToMRAM(item->data, dataIn, bytesToCopy);
        return true;
    }  
}


bool k_AddSettingIfNotExistent(char* path, SettingsType type, void* defaultValue, uint32_t dataSize){

    if(strlen(path) > SETTINGS_MAX_NAME_LENGHT){
        LOG_E("Setting name (%s) is too large! %d vs %d", path , strlen(path),SETTINGS_MAX_NAME_LENGHT);
        return false;
    }

    if(k_GetROSettingsBlock(path, type) != NULL) return false; //If exists just leave
   
    uint32_t settingsSize = 0;
    k_settingsItem* currentItem = (k_settingsItem*)k_GetStorageReadOnlyAddressForApp(k_getAppByName("Settings"), &settingsSize);
    if(currentItem==NULL) return false;

    uint32_t howManySettingsFit = settingsSize / sizeof(k_settingsItem);
   
    uint32_t currentCount=0;
    while(currentCount < howManySettingsFit){
        if(currentItem->cookie != SETTINGS_COOKIE){
            //Found an empty item

            LOG_W("Added new setting: %s", path);
            k_settingsItem tempItem;
            strcpy((char*)tempItem.name, path);
            tempItem.cookie = SETTINGS_COOKIE;
            tempItem.dataType = type;
            memcpy(tempItem.data, defaultValue, dataSize);
            k_copyToMRAM((uint8_t*)currentItem, (uint8_t*)&tempItem, sizeof(k_settingsItem));
            return true;
        }

        currentCount++;
        currentItem++;
    }

    //Couldn't find space
    return false;
}

void k_ResetAllSetings(){
    uint32_t settingsSize = 0;
    k_settingsItem* currentItem = (k_settingsItem*)k_GetStorageReadOnlyAddressForApp(k_getAppByName("Settings"), &settingsSize);
    if(currentItem==NULL) return;

    k_setToMRAM((uint8_t*)currentItem, 0, settingsSize);
}


uint32_t k_GetSettingUInt32(char* path, uint32_t defaultValue){
    uint32_t out;
    if(k_GetSettingGeneric(path,Uint32, &out, sizeof(out))){
        return out;
    }else{
        return defaultValue;
    }
}

int32_t k_GetSettingInt32(char* path, int32_t defaultValue){
    int32_t out;
    if(k_GetSettingGeneric(path,Int32, &out, sizeof(out))){
        return out;
    }else{
        return defaultValue;
    }
}

bool k_GetSettingBool(char* path, bool defaultValue){
    bool out;
    if(k_GetSettingGeneric(path, Bool,&out, sizeof(out))){
        return out;
    }else{
        return defaultValue;
    }
}

uint32_t k_SetSettingUInt32(char* path, uint32_t valueToSet){
    return k_SetSettingGeneric(path,Uint32, &valueToSet, sizeof(valueToSet));
}

int32_t k_SetSettingInt32(char* path, int32_t valueToSet){
    return k_SetSettingGeneric(path,Int32, &valueToSet, sizeof(valueToSet));
}

bool k_SetSettingBool(char* path, bool valueToSet){
    return k_SetSettingGeneric(path,Bool, &valueToSet, sizeof(valueToSet));
}

void* k_EnumerateNextSettingEntry(char* beginningFilter, void* lastTokenOrNULL, char* nameOut, SettingsType* typeOut){

    uint32_t settingsSize = 0;
    k_settingsItem* baseAddress=(k_settingsItem*)k_GetStorageReadOnlyAddressForApp(k_getAppByName("Settings"), &settingsSize);
    k_settingsItem* currentSettingsItem = (k_settingsItem*)lastTokenOrNULL;

    uint32_t howManySettingsFit = settingsSize / sizeof(k_settingsItem);

    if(currentSettingsItem == NULL){
        currentSettingsItem = baseAddress;
        if(currentSettingsItem == NULL) return NULL; //Couldn't get a pointer, just stop enumerating (returning NULL as the token)
    }

    while(currentSettingsItem->cookie==SETTINGS_COOKIE && (currentSettingsItem-baseAddress < howManySettingsFit)){
        if(strncmp(currentSettingsItem->name, beginningFilter, strlen(beginningFilter))==0){
            strcpy(nameOut, currentSettingsItem->name);
            *typeOut = currentSettingsItem->dataType;
            return currentSettingsItem+1;
        }else{
            currentSettingsItem++;
        }
    }

    //Not a valid item anymore
    return NULL; //Finish enumeration    
}