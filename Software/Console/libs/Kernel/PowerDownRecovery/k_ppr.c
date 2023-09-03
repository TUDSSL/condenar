#include "PowerDownRecovery.h"
#include "Kernel.h"
#include "Helpers/MRAM.h"
#include "Helpers/Misc.h"
#include "string.h"
#include "Helpers/TimeKeeper.h"
#include "Helpers/RNG.h"
#include "Helpers/MemWatcher.h"
#include "Screen/Screen.h"
#include "k_ppr.h"

const char* currentCompilationTimestamp = __TIMESTAMP__;

//MRAM checkpoing structure (MRAM_SAVESTATE region)
__attribute__((__section__(".SaveState"))) uint32_t lastCheckpointID;
__attribute__((__section__(".SaveState"))) struct MRAM_checkpoint_data checkpointData[2];

//Defines in inline assembly macros from https://stackoverflow.com/questions/52525630/define-in-inline-assembly-in-gcc
#define STR(x) #x
#define XSTR(s) STR(s)

volatile bool isRecoveringFromACheckpoint = false;

bool k_isRecoveringFromACheckpoint(){
    return isRecoveringFromACheckpoint;
}

volatile uint32_t checkpointRestoreStartTime; //Can't put this on the stack because it will get replaced when the context is restored
void k_tryRecoverFromPowerDown(){

    if(!k_GetSettingBool("/Intermittency/Checkpoint recovery enabled",true)){
        LOG_I("Checkpoint restore disabled. Skipping...");
        return;
    }

    k_input_state state;
    k_GetInput(&state, false);
    if(state.triggerLeft && state.triggerRight && state.buttonA){
        LOG_I("Buttom combination pressed, skipping checkpoint...");
        return;
    }

    if(!k_isCheckpointValid(lastCheckpointID)){
        LOG_I("Couldn't find a valid checkpoint. Doing a normal start...");
        return;
    }
    
    LOG_I("Found a valid checkpoint (%d). Restoring it...", lastCheckpointID);

    k_WaitForPendingScreenFrames();
    
    checkpointRestoreStartTime = millis();
    am_hal_interrupt_master_disable();

    MemWatcher_SetEnabled(false);
    k_ppr_restoreHeap(lastCheckpointID);        
    MemWatcher_SetEnabled(true);
    k_ppr_restoreLinkerSections(lastCheckpointID);



    uint32_t* realStackPtr = (uint32_t*)g_pui32Stack;
    uint32_t* fakeStackPtr = (uint32_t*)checkpointData[lastCheckpointID].stack;
    register uint32_t* regSavePtr __asm__("r0") = checkpointData[lastCheckpointID].fixedData.registerData;

    __asm__ volatile ("" ::: "memory");

    //Restore stack
     __asm__ volatile (
        "MOV r2, #0 \n\t"
        "stack_copy_loop_restore:     \n\t"
        "LDR r3, [%1], #4     \n\t"
      //  "MOV r3, #0xCA     \n\t"
        "STR r3, [%0], #4     \n\t"
        "ADD r2, r2, #1       \n\t"
        "cmp     r2, "XSTR(APP_STACK_SIZE/4)"     \n\t"
        "blt stack_copy_loop_restore    \n\t"       
        : 
        : "r" (realStackPtr), "r" (fakeStackPtr)
        : "r3", "r2", "memory"
        );

    //Restore registers
    __asm__ volatile (
        "LDR r1, [r0], #4     \n\t"
        "LDR r2, [r0], #4     \n\t"
        "LDR r3, [r0], #4     \n\t"
        "LDR r4, [r0], #4     \n\t"
        "LDR r5, [r0], #4     \n\t"
        "LDR r6, [r0], #4     \n\t"
        "LDR r7, [r0], #4     \n\t"
        "LDR r8, [r0], #4     \n\t"
        "LDR r9, [r0], #4     \n\t"
        "LDR r10, [r0], #4     \n\t"
        "LDR r11, [r0], #4     \n\t"
        "LDR r12, [r0], #4     \n\t"
        "LDR r13, [r0], #4     \n\t"
        "LDR r14, [r0], #4     \n\t"
        //We just pushed all registers (14 regs) to the stack (everything but sp)
        //"ADD r1, r13, #56\n\t" //Calculate the value of the SP before pushing the other registers
        //"PUSH {r1}    \n\t" //Push that calculated value to the stack
        : 
        : "r" (regSavePtr)
        : "memory"
        );

    //Jump!    
    __asm__ volatile (
        "b restorePoint  \n\t"
        );
        
}

uint32_t tempBuffer[14];
uint32_t stackBuffer2[APP_STACK_SIZE/4];

#pragma GCC optimize("O0")
void k_storeCurrentState(){


    uint32_t startTime = millis();

    //Screen_WaitForFrameDone();

    am_hal_interrupt_master_disable();

    k_memRegions modifiedRegions;
    k_MemWatcher_GetModifiedRegions(&modifiedRegions);
    k_MemWatcher_ResetRegions();   

    uint32_t newCheckpointID = lastCheckpointID == 0 ? 1 : 0;  

    k_markCheckpointAsInvalid(newCheckpointID);  

    k_ppr_checkpointHeap(newCheckpointID); 
    k_ppr_checkpointLinkerSections(newCheckpointID);

    //Store registers
    register uint32_t* regSavePtr __asm__("r0") = tempBuffer;
    uint32_t* realStackPtr = g_pui32Stack;
    uint32_t* fakeStackPtr = (uint32_t*)stackBuffer2;

    __asm__ volatile (
        "STR r1, [r0], #4     \n\t"
        "STR r2, [r0], #4     \n\t"
        "STR r3, [r0], #4     \n\t"
        "STR r4, [r0], #4     \n\t"
        "STR r5, [r0], #4     \n\t"
        "STR r6, [r0], #4     \n\t"
        "STR r7, [r0], #4     \n\t"
        "STR r8, [r0], #4     \n\t"
        "STR r9, [r0], #4     \n\t"
        "STR r10, [r0], #4     \n\t"
        "STR r11, [r0], #4     \n\t"
        "STR r12, [r0], #4     \n\t"
        "STR r13, [r0], #4     \n\t"
        "STR r14, [r0], #4     \n\t"
        : 
        : "r" (regSavePtr)
        : "memory"
        );
    
    __asm__ volatile (
        "MOV r2, #0 \n\t"
        "stack_copy_loop_save:     \n\t"
        "LDR r3, [%0], #4     \n\t"
        "STR r3, [%1], #4     \n\t"
        "ADD r2, r2, #1       \n\t"
        "cmp     r2, "XSTR(APP_STACK_SIZE/4)"     \n\t"
        "blt stack_copy_loop_save    \n\t"       
        : 
        : "r" (realStackPtr), "r" (fakeStackPtr)
        : "r3", "r2", "memory"
        );    

    __asm__ ("restorePoint:"); //When a checkpoint is restored everything starts here again

    am_hal_interrupt_master_enable();

    if(!k_isCheckpointValid(newCheckpointID)){ //This will only execute if we are creating the checkpoint (it was invalidated before)

        //Store temp stack and register buffers to MRAM and mark the checkpoint as valid
        k_copyToMRAM((uint8_t*)checkpointData[newCheckpointID].stack,(uint8_t*)stackBuffer2,APP_STACK_SIZE);
        k_copyToMRAM((uint8_t*)checkpointData[newCheckpointID].fixedData.registerData,(uint8_t*)tempBuffer, 14*4);
        k_markCheckpointAsValid(newCheckpointID);
        k_copyToMRAM((uint8_t*)&lastCheckpointID,(uint8_t*)&newCheckpointID,sizeof(lastCheckpointID));

        uint32_t endTime = millis();
        LOG_I("Checkpoint (%d) saved in %dms",lastCheckpointID,endTime-startTime);
    }else{ //We got here without invalidating the checkpoint (we jumped to the label), so k_isCheckpointValid will return true
        //Here lastCheckpointID should be the same as newCheckpointID
        uint32_t endTime = millis();
        LOG_I("Checkpoint (%d) restored in %dms", lastCheckpointID, endTime-startTime);

        k_app* currentApp = k_getCurrentApp();
        if(currentApp!= NULL && currentApp->onCheckpointRestore!=NULL){
            isRecoveringFromACheckpoint=true;
            currentApp->onCheckpointRestore(currentApp);
            isRecoveringFromACheckpoint=false;
        }        
    }

    __asm__ volatile ("" ::: "memory");
}

bool k_isCheckpointValid(uint32_t id){
    if(id>1) return false; //We only support 2 checkpoints (0 and 1)

    pdr_header header;
    k_getCheckpointHeader(&header, id);

    if(header.magicNumber != PDR_MAGIC_NUMBER) return false;

    #ifndef PREVENT_COMPILATION_INVALIDATION
    if(strcmp(header.compilationTimestamp,currentCompilationTimestamp)!=0) return false;
    #endif
    
    return header.checkpointValid;
}


void k_markCheckpointAsValid(uint32_t id){
    pdr_header header;
    header.checkpointValid = true;
    header.magicNumber = PDR_MAGIC_NUMBER;
    strcpy(header.compilationTimestamp, currentCompilationTimestamp);

    k_setCheckpointHeader(&header, id);
}

void k_markCheckpointAsInvalid(uint32_t id){
    pdr_header header;
    header.checkpointValid = false;
    header.magicNumber = PDR_MAGIC_NUMBER;
    strcpy(header.compilationTimestamp, currentCompilationTimestamp);

    k_setCheckpointHeader(&header, id);
}

void k_ppr_restoreHeap(uint32_t id){
    uint8_t* heapPtr = k_getHeapPtr();
    uint32_t heapSize = k_getHeapSize();
    if(heapPtr==NULL) return;

    memcpy(heapPtr, checkpointData[id].heap, heapSize);
}

void k_ppr_checkpointHeap(uint32_t id){
    uint8_t* heapPtr = k_getHeapPtr();
    uint32_t heapSize = k_getHeapSize();
    if(heapPtr==NULL) return;

    k_copyToMRAM(checkpointData[id].heap, heapPtr, heapSize);
}


void k_ppr_restoreLinkerSections(uint32_t id){
    if(id==0){
        memcpy((uint8_t*)&_scheckpointedData, (uint8_t*)&_mramCheckpointedDataStart, (uint32_t)&_sizecheckpointedData);
        memcpy((uint8_t*)&_scheckpointedBss, (uint8_t*)&_mramCheckpointedBssStart, (uint32_t)&_sizecheckpointedBss);
    }else{
        memcpy((uint8_t*)&_scheckpointedData, (uint8_t*)&_mramCheckpointedDataStart2, (uint32_t)&_sizecheckpointedData);
        memcpy((uint8_t*)&_scheckpointedBss, (uint8_t*)&_mramCheckpointedBssStart2, (uint32_t)&_sizecheckpointedBss);
    }

}

void k_ppr_checkpointLinkerSections(uint32_t id){
    if(id==0){
        k_copyToMRAM((uint8_t*)&_mramCheckpointedDataStart, (uint8_t*)&_scheckpointedData, (uint32_t)&_sizecheckpointedData);
        k_copyToMRAM((uint8_t*)&_mramCheckpointedBssStart, (uint8_t*)&_scheckpointedBss, (uint32_t)&_sizecheckpointedBss);
    }else{
        k_copyToMRAM((uint8_t*)&_mramCheckpointedDataStart2, (uint8_t*)&_scheckpointedData, (uint32_t)&_sizecheckpointedData);
        k_copyToMRAM((uint8_t*)&_mramCheckpointedBssStart2, (uint8_t*)&_scheckpointedBss, (uint32_t)&_sizecheckpointedBss);
    }
}


void k_getCheckpointHeader(pdr_header* headerOut, uint32_t id){
    memcpy(headerOut, (uint8_t*)&checkpointData[id].header, sizeof(pdr_header));
}

void k_setCheckpointHeader(pdr_header* header, uint32_t id){
    k_copyToMRAM((uint8_t*)&checkpointData[id].header, (uint8_t*)header, sizeof(pdr_header));
}
