/* 
 *  12-11-2022 - Adapted from:
 *  Created on: Mar 31, 2020
 *      Author: TU Delft Sustainable Systems Laboratory
 *     License: MIT License
 */


#include "MemWatcher.h"
#include "Kernel.h"
#include "am_mcu_apollo.h"
#include "Misc.h"
#include "string.h"

#define REGION_NUM 8
#define REGIONSIZE ARM_MPU_REGION_SIZE_8KB  //16 = 2^16 = 64KB (65,536 B)
#define REGIONMASK ((2UL << REGIONSIZE) - 1)
#define SUBREGIONSIZE (REGIONSIZE - 3)
#define SUBREGIONMASK ((2UL << (SUBREGIONSIZE)) - 1)

#define REGIONSIZE_BYTES (2UL << REGIONSIZE)
#define SUB_REGIONSIZE_BYTES (2UL << SUBREGIONSIZE)
//Threfore each subregion (8 in each region) will be 8KB (8,192 B)

uint8_t regionTracker[REGION_NUM];
uint32_t startAddress;

//APOLLO4 MPU BASE ADDRESS GRANULARITY: 32B   (Test this by writing 0xFFFFFFE0 and reading back from the RBAR register (and see if any bits were removed))
//Use 64KB regions to cover the whole TCM (384KB)

void MemWatcher_Initialize(){
    startAddress = (uint32_t)k_getHeapPtr();

    uint32_t regionSizeBytes =  2 << (REGIONSIZE);

    for(int cRegion=0;cRegion<REGION_NUM;cRegion++){
        MPU->RBAR = ARM_MPU_RBAR(cRegion, startAddress + (cRegion * regionSizeBytes));
        MPU->RASR = ARM_MPU_RASR_EX(false, ARM_MPU_AP_RO, 
                                ARM_MPU_ACCESS_(0,true,false,true),
                                0, REGIONSIZE);
    }    

    MemWatcher_SetEnabled(true);
}

void k_MemWatcher_ResetRegions(){
    //We just disable each subregion when written, so just re-enable all of them
    for(int cRegion=0;cRegion<REGION_NUM;cRegion++){
        MPU->RNR = cRegion; //Select the region to modify
        MPU->RASR = MPU -> RASR & (~MPU_RASR_SRD_Msk); //Set all the subregion disable bits to 0
    }    
    memset(regionTracker,0,sizeof(regionTracker));
}

void k_MemWatcher_GetModifiedRegions(k_memRegions* k_memRegions){
    memcpy(k_memRegions->regionModifiedStatus, regionTracker, sizeof(regionTracker));
}

void MemWatcher_SetEnabled(bool enabled){
    if(enabled){
        ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk); //Allow access to non defined regions in the MPU
    }else{
        ARM_MPU_Disable();
    }        
}



void memTrackingHandler(uint32_t stackptr) {
  // get the address causing the issue MIGHT BE INVALID!
  uint32_t address = SCB->MMFAR;
  address -= startAddress;
  // Check if MPU caused the interrupt. (MMFAR is shared with BFAR!)
  if (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) {
    // Reset CFSR otherwise MMFAR remains the same.
    SCB->CFSR |= SCB->CFSR;

    // Apply mask to get the subregion that triggered the handler.
    address &= ~SUBREGIONMASK;
    // Retrieve the region.
    uint8_t regionAndSubregion = address / SUB_REGIONSIZE_BYTES;
    uint8_t region = regionAndSubregion / 8;
    uint8_t subRegion = regionAndSubregion % 8;

    //LOG_I("Region %d modified",regionAndSubregion);

    // Count the write
    regionTracker[region] |= (1 << (7-subRegion));

    // Select the correct region
    MPU->RNR = region;
    // Disable the region.
    MPU->RASR &= ~MPU_RASR_ENABLE_Msk;

    MPU->RASR |= MPU_RASR_ENABLE_Msk | (1UL << (subRegion + 8));
  }
}

void __attribute__((naked)) MemManage_Handler(void) {
  __asm(" push    {r0,lr}");
  __asm(" tst     lr, #4");
  __asm(" itet    eq");
  __asm(" mrseq   r0, msp");
  __asm(" mrsne   r0, psp");
  __asm(" addseq  r0, r0, #8");
  __asm(" bl      memTrackingHandler");
  __asm(" pop     {r0,pc}");
}