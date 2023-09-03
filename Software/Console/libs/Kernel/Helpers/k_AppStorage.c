#include "Kernel.h"
#include "Misc.h"
#include "string.h"
#include "MRAM.h"
#include "AppStorage.h"

#define APP_STORAGE_MAGICNUMBER 0x68694203

bool appStorageReady = false;

/* STRUCTURE OF APP STORAGE MEMORY:
AppStorageMemory (magicNumber and number of chunks)

AppStorageMemoryItem (appName and dataLength)
X (dataLength) bytes of data

AppStorageMemoryItem+1 (appName and dataLength)
X (dataLength) bytes of data

.................*/

struct AppStorageMemoryItem{
    char appName[MAX_APPNAME_LENGTH];
    uint32_t dataLength;
};

struct AppStorageMemory{
    uint32_t magicNumber;
    uint32_t itemCount;
};

bool k_AppStorageTryLocateOrAddChunksForApps();
bool k_AppStorageAddChunk(char* name, uint32_t size);
struct AppStorageMemoryItem* k_AppStorageFindChunk(char* name, uint32_t size);
void k_AppStorageCompactChunks();

extern uint32_t __appStorageStart;
extern uint32_t __appStorageEnd;

struct AppStorageMemory* k_appStorageMRAMRegion; //Filled in the initialization
uint32_t k_appStorageMRAMRegionSize;

extern uint32_t __testN;
extern uint32_t __testS;

uint8_t* k_GetStorageReadOnlyAddressForApp(k_app* app, uint32_t* sizeOut){
    if(!appStorageReady){
        return NULL;
    }

    if(app==NULL) return NULL;

    struct AppStorageMemoryItem* chunk = k_AppStorageFindChunk(app->name, app->requestedStorageBytes);
    if(chunk == NULL) return NULL;
 
    if(sizeOut!=NULL) *sizeOut= app->requestedStorageBytes;
    //Skip the header
    return (uint8_t*)chunk + sizeof(struct AppStorageMemoryItem);
}

void k_WriteToAppStorage(k_app* app, uint32_t storageOffset, uint8_t* data,  uint32_t size){
    if(!appStorageReady){
        LOG_W("Cancelling write to app storage because it is not initialized!");
        return;
    }

    if(app==NULL) return;

    if(storageOffset + size > app->requestedStorageBytes){
        LOG_W("App tried to write to app storage beyond the limits! %s",app->name);
        return;
    }

    struct AppStorageMemoryItem* chunk = k_AppStorageFindChunk(app->name, app->requestedStorageBytes);
    if(chunk == NULL){
        LOG_W("Cancelling app storage write because chunk was not found! %s",app->name);
        return;
    }

    uint8_t* mramPtr= (uint8_t*)chunk + sizeof(struct AppStorageMemoryItem);
    k_copyToMRAM(mramPtr + storageOffset, data, size);
}

void k_InitializeStorageForApps(){

    k_appStorageMRAMRegion = (struct AppStorageMemory*)&__appStorageStart;
    k_appStorageMRAMRegionSize = (uint32_t)&__appStorageEnd- (uint32_t)&__appStorageStart;

    if(k_appStorageMRAMRegionSize < sizeof(struct AppStorageMemory)){
        LOG_W("Not enough assigned space (%d) for the APP_STORAGE", k_appStorageMRAMRegionSize);
        return;
    }

    if(k_appStorageMRAMRegion->magicNumber!=APP_STORAGE_MAGICNUMBER){
        LOG_W("App storage not initalized! Initializing it now...");
        k_AppStorageReset();
    }else{
        LOG_I("Found old app storage index. Block count: %d", k_appStorageMRAMRegion->itemCount);
    }
 
    appStorageReady = k_AppStorageTryLocateOrAddChunksForApps();
    if(!appStorageReady){ //If still not ready (probably couldn't fit something)
        LOG_W("Couldn't fit all app storage chunks in MRAM! Trying to compact it...");
        k_AppStorageCompactChunks(); //Try to compact chunks (look for unused apps)
        appStorageReady = k_AppStorageTryLocateOrAddChunksForApps();
        if(!appStorageReady){
            LOG_E("Still couldn't fit all app storage chunks in MRAM! Some apps might not have app storage!!!!");
        }
    }
}

void k_AppStorageCompactChunks(){
    uint32_t appCount;
    k_app** apps = k_getAppList(&appCount);

    int blocksRemovedCount=0;

startAgain:;

    LOG_I("Starting app storage chunk compacting iteration...");
    DelayMS(1000); //Just in case something bricks, this should slow down the use of flash erase cycles

    uint8_t* currentMemoryAddress = (uint8_t*)k_appStorageMRAMRegion + sizeof(struct AppStorageMemory);

    for(int i=0;i<k_appStorageMRAMRegion->itemCount;i++){
        if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) > (uint8_t*)&__appStorageEnd) 
        {
            LOG_E("Found out of bounds item when compacting. Giving up and resetting the storage...");
            k_AppStorageReset();
            return;  //We are out of the region (probably the max size of the storage got smaller). Just reset everything                
        }
        
        struct AppStorageMemoryItem* item = (struct AppStorageMemoryItem*)currentMemoryAddress;
        bool appExists = false;
        for(int k=0;k<appCount;k++){ //Find if there is an app with the settings of the chunk
            if(apps[k]->requestedStorageBytes == item->dataLength && strcmp(apps[k]->name,item->appName)==0){
                appExists=true;
                break;
            }
        }

        if(!appExists){

            LOG_W("Found app that doesn't exit while compacting: %s Size=%d. Removing it...",item->appName,item->dataLength);

            if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) + item->dataLength > (uint8_t*)&__appStorageEnd)
            {
                LOG_E("Found out of bounds item when compacting. Giving up and resetting the storage...");
                k_AppStorageReset();
                return;  //We are out of the region (probably the max size of the storage got smaller). Just reset everything                
            }
            
            uint8_t* nextChunkAddress= currentMemoryAddress + sizeof(struct AppStorageMemoryItem) + item->dataLength;
            k_copyToMRAM(currentMemoryAddress,nextChunkAddress, (uint32_t)&__appStorageEnd - (uint32_t)nextChunkAddress); //Copy the remaining blocks forward
            //This is not very efficient but should be fine for now, since this function is only called when there isn't enough space

            //Decreate the total block count by 1 and start again
            struct AppStorageMemory temp = *k_appStorageMRAMRegion;    
            temp.itemCount--;
            k_copyToMRAM((uint8_t*)k_appStorageMRAMRegion, (uint8_t*)&temp, sizeof(struct AppStorageMemory));
            goto startAgain;
        }
        
        currentMemoryAddress += sizeof(struct AppStorageMemoryItem);
        currentMemoryAddress += item->dataLength;
    }

    LOG_I("App storage compacting finished. Blocks removed: %d", blocksRemovedCount);  
    
}

void k_AppStorageReset(){
    struct AppStorageMemory temp;    
    temp.itemCount = 0;
    temp.magicNumber = APP_STORAGE_MAGICNUMBER;
    k_copyToMRAM((uint8_t*)k_appStorageMRAMRegion, (uint8_t*)&temp, sizeof(struct AppStorageMemory));
}

bool k_AppStorageTryLocateOrAddChunksForApps(){
    uint32_t appCount;
    k_app** apps = k_getAppList(&appCount);
    for(int i=0;i<appCount;i++){
        LOG_W("App %s requested %d bytes of storage",apps[i]->name,apps[i]->requestedStorageBytes);
        if(apps[i]->requestedStorageBytes>0){
            struct AppStorageMemoryItem* item = k_AppStorageFindChunk(apps[i]->name,apps[i]->requestedStorageBytes);
            if(item==NULL){
                bool success= k_AppStorageAddChunk(apps[i]->name,apps[i]->requestedStorageBytes);
                if(success){
                    LOG_W("Created app storage chunk for app %s. Size=%d",apps[i]->name,apps[i]->requestedStorageBytes);
                }else{
                    LOG_E("Failed to create app storage chunk for app %s. Size=%d",apps[i]->name,apps[i]->requestedStorageBytes);
                    return false;
                }
            }else{
                LOG_I("Loaded app storage chunk for app %s. Size=%d",apps[i]->name,apps[i]->requestedStorageBytes);
            }
        }
    }
    return true;
}

//Tries to find an existing chunk with that name and size. If not found, returns NULL
struct AppStorageMemoryItem* k_AppStorageFindChunk(char* name, uint32_t size){
    uint8_t* currentMemoryAddress = (uint8_t*)k_appStorageMRAMRegion + sizeof(struct AppStorageMemory);

    for(int i=0;i<k_appStorageMRAMRegion->itemCount;i++){
        if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) > (uint8_t*)&__appStorageEnd) return NULL; //We are out of the region, so this chunk does not exist
        struct AppStorageMemoryItem* item = (struct AppStorageMemoryItem*)currentMemoryAddress;     
        if(item->dataLength == size && strcmp(name,item->appName)==0){
            //THIS CHECK NEEDS TO HAPPEN NOW TO MAKE SURE WE ARE NOT READING MEMORY OUT OF THE RANGE!!!
            if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) + item->dataLength > (uint8_t*)&__appStorageEnd) return NULL; //We are out of the region, so this chunk does not exist
            return item;
        }
        currentMemoryAddress += sizeof(struct AppStorageMemoryItem);
        currentMemoryAddress += item->dataLength;
    }
    return NULL;
}

//Returns whether there was enough space too add the new chunk
bool k_AppStorageAddChunk(char* name, uint32_t size){

    uint8_t* currentMemoryAddress = (uint8_t*)k_appStorageMRAMRegion + sizeof(struct AppStorageMemory);

    for(int i=0;i<k_appStorageMRAMRegion->itemCount;i++){
        if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) > (uint8_t*)&__appStorageEnd) return false; //We are out of the region, so there isn't space for a new one
        //THIS CHECK NEEDS TO HAPPEN NOW TO MAKE SURE WE ARE NOT READING MEMORY OUT OF THE RANGE!!!
        struct AppStorageMemoryItem* item = (struct AppStorageMemoryItem*)currentMemoryAddress;
        if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) + item->dataLength > (uint8_t*)&__appStorageEnd) return false; //We are out of the region, so there isn't space for a new one

        currentMemoryAddress += sizeof(struct AppStorageMemoryItem);
        currentMemoryAddress += item->dataLength;
    } //Go through every chunk to find the next item address

    if(currentMemoryAddress + sizeof(struct AppStorageMemoryItem) + size > (uint8_t*)&__appStorageEnd) return false; //Not enough space

    //Allocate new chunk and initialize memory to 0
    struct AppStorageMemoryItem newItem;
    strcpy(newItem.appName, name);
    newItem.dataLength = size;
    k_copyToMRAM(currentMemoryAddress, (uint8_t*)&newItem, sizeof(struct AppStorageMemoryItem));
    k_setToMRAM(currentMemoryAddress + sizeof(struct AppStorageMemoryItem),0x00, size);

    //Update the total item count
    struct AppStorageMemory temp = *k_appStorageMRAMRegion;    
    temp.itemCount++;
    k_copyToMRAM((uint8_t*)k_appStorageMRAMRegion, (uint8_t*)&temp, sizeof(struct AppStorageMemory));

    DelayMS(100); //Just in case something bricks, this should slow down the use of flash erase cycles

    return true;
}
