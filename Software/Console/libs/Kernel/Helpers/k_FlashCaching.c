#include "Flash.h"
#include "Kernel.h"
#include "Helpers/Misc.h"



#ifdef FLASH_CACHING_ENABLED

#define CACHE_LINE_COUNT 8
#define CACHE_LINE_SIZE_BYTES 32

CHECKPOINT_EXCLUDE_BSS struct CacheLine{
	uint32_t addressStart;
	uint8_t cachedData[CACHE_LINE_SIZE_BYTES];
} romCache[CACHE_LINE_COUNT];

//In order to get evicted
CHECKPOINT_EXCLUDE_BSS struct CacheLine* romCachePointers[CACHE_LINE_COUNT];

void InitFlashCaching(){
    for(int i=0;i<CACHE_LINE_COUNT;i++){
		romCachePointers[i] = &romCache[i];
		romCachePointers[i]->addressStart = (uint32_t)-1;
	}
}


uint8_t* GetCacheableAddress(uint32_t address, int readSize){

    IncreaseMetrics("Flash reads", 1); 

    if(address < MSPI_XIP_BASE_ADDRESS){
        LOG_E("Flash cachable address not in XIP range: %d", address);
        SpinLoop();
    }

	uint32_t remainingAddress = address & (CACHE_LINE_SIZE_BYTES-1);
	uint32_t cacheableAddressStart = address - remainingAddress;

	//LOG_I("----- Cacheable request @%d. Base address: %d Remaining address: %d", address, cacheableAddressStart, remainingAddress);

	if(remainingAddress > CACHE_LINE_SIZE_BYTES - readSize)
	{		//Not safe to cache because a unalgined int could be there
		LOG_I("Address not safe to cache: %d", cacheableAddressStart);
		goto notCached; 
	}
	 

	for(int i=CACHE_LINE_COUNT-1;i>=0;i--){
		if(romCachePointers[i]->addressStart==cacheableAddressStart)
		{
			//LOG_I("Cache hit on: %d", romCachePointers[i]->addressStart);
            IncreaseMetrics("Cache hits", 1);
			return (uint8_t*)(romCachePointers[i]->cachedData + remainingAddress);
		}	
	}

	//LOG_I("Evicting and adding cache line: %d",cacheableAddressStart);

	//Cacheable but not found. Cache it
	struct CacheLine* lineToEvictAndUse = romCachePointers[0];
	for(int i=0;i<CACHE_LINE_COUNT - 1;i++){
		romCachePointers[i] = romCachePointers[i+1];		
	}
	lineToEvictAndUse->addressStart = cacheableAddressStart;
	memcpy(lineToEvictAndUse->cachedData, cacheableAddressStart, CACHE_LINE_SIZE_BYTES);
	romCachePointers[CACHE_LINE_COUNT-1] = lineToEvictAndUse;

    IncreaseMetrics("Cache misses", 1);

	return (uint8_t*)(lineToEvictAndUse->cachedData + remainingAddress);

notCached:
	return (uint8_t*)((uint8_t*)address);
}
#else 
void InitFlashCaching(){}
#endif