#include "MRAM.h"
#include "string.h"
#include "am_mcu_apollo.h"
#include "Misc.h"

#define MAX_MRAM_ADDRESS 0x0 + 2097152  //Used for detecting whether a pointer is in MRAM or in RAM

//MRAM writes need to be 16 byte aligned (and with a 16 byte length). That is the whole purpose of this function
void k_copyToMRAM(uint8_t* dst, uint8_t* src, uint32_t size){

    int result=0;
   
    if(((uint32_t)dst & 0xF) != 0){ //This means that the start is not aligned
        uint8_t buffer[16] __attribute__((aligned(4))); //Our 16 byte buffer

        uint8_t remainder = (uint32_t)dst & 0xF;
        uint8_t bytesToProgram = 16-remainder;
        if(bytesToProgram > size) bytesToProgram = size;
        uint8_t* alignedDestination = dst - remainder;

        //Copy to our buffer the contents of the MRAM of the previous 16 byte block
        memcpy(buffer, alignedDestination, 16);
        //Copy the new contents to the buffer
        memcpy(buffer + remainder,src, bytesToProgram);

        //Program this first chunk
        result|=am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY,(uint32_t*)buffer,(uint32_t*)alignedDestination,16/4);

        src += bytesToProgram;
        dst += bytesToProgram;
        size -= bytesToProgram;
    }

    if(size==0) goto exit;

    //We know that src and dst are not aligned

    uint8_t extraNotAlignedBytes = size & 0xF; //Bytes that wouldn't be aligned
    uint32_t bytesToCopyMain = size - extraNotAlignedBytes;

    //SRC also needs to be aligned to word boundary, so if it is not, SPECIAL CASE!!!!
    //If SRC is also in MRAM, we also have to do this :(
    if(((uint32_t)src & 0x03) != 0 || src < (uint8_t*)MAX_MRAM_ADDRESS){
        //SRC not aligned, double copy first to a buffer in the stack :(
        
        uint8_t tempBuffer[256] __attribute__((aligned(4))); //ALWAYS MULTIPLE OF 16!!!!!
        uint32_t bytesToCopyWordAlign = bytesToCopyMain;

        while(bytesToCopyWordAlign > 0){
            uint32_t bytesToCopyInThisLoop=MIN(sizeof(tempBuffer),bytesToCopyWordAlign);
            memcpy(tempBuffer,src, bytesToCopyInThisLoop);
            result|=am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY,(uint32_t*)tempBuffer,(uint32_t*)dst,bytesToCopyInThisLoop/4); //bytesToCopyInThisLoop is always a multiple of 16 since bytesToCopyMain and sizeof(tempBuffer) are

            dst += bytesToCopyInThisLoop;
            src += bytesToCopyInThisLoop;
            bytesToCopyWordAlign-=bytesToCopyInThisLoop;
        }
    }else{
        result|=am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY,(uint32_t*)src,(uint32_t*)dst,bytesToCopyMain/4); //SRC is aligned, just copy as usual
        src += bytesToCopyMain;
        dst += bytesToCopyMain;
    }

    size -= bytesToCopyMain;

    if(extraNotAlignedBytes > 0){ //We still need to program not aligned bytes, kind of repeat what we did on top
        uint8_t buffer[16] __attribute__((aligned(4))); //Our 16 byte buffer

        //Copy to our buffer the contents of the MRAM of the next 16 byte block
        memcpy(buffer, dst, 16);
        //Copy the new contents to the buffer
        memcpy(buffer ,src, size);

        //Program this last chunk
        result|=am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY,(uint32_t*)buffer,(uint32_t*)dst,16/4);
    }

exit:;
    if(result!=0) 
    {
        LOG_E("k_copyToMRAM finished with a non 0 (%d) code!!!!!",result);
    }

}



//MRAM writes need to be 16 byte aligned (and with a 16 byte length). That is the whole purpose of this function
void k_setToMRAM(uint8_t* dst, uint8_t value, uint32_t size){

    int result=0;

    if(((uint32_t)dst & 0xF) != 0){ //This means that the start is not aligned
        uint8_t buffer[16] __attribute__((aligned(4))); //Our 16 byte buffer

        uint8_t remainder = (uint32_t)dst & 0xF;
        uint8_t bytesToProgram = 16-remainder;
        if(bytesToProgram > size) bytesToProgram = size;
        uint8_t* alignedDestination = dst - remainder;

        //Copy to our buffer the contents of the MRAM of the previous 16 byte block
        memcpy(buffer, alignedDestination, 16);
        //Copy the new contents to the buffer
        memset(buffer + remainder,value, bytesToProgram);

        //Program this first chunk
        result |= am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY,(uint32_t*)buffer,(uint32_t*)alignedDestination,16/4);

        dst += bytesToProgram;
        size -= bytesToProgram;
    }

    if(size==0) goto exit;

    //We know that src and dst are not aligned

    uint8_t extraNotAlignedBytes = size & 0xF; //Bytes that wouldn't be aligned
    uint32_t bytesToCopyMain = size - extraNotAlignedBytes;
    uint32_t valueToSetInU32 = (value << 24) | (value << 16) | (value <<8) | (value);
    result |= am_hal_mram_main_fill(AM_HAL_MRAM_PROGRAM_KEY,valueToSetInU32,(uint32_t*)dst,bytesToCopyMain/4);

    if(extraNotAlignedBytes > 0){ //We still need to program not aligned bytes, kind of repeat what we did on top
        
        dst += bytesToCopyMain; //Adjust pointers from the previous step (the main aligned copy). Done here to be more efficient in some cases
        size -= bytesToCopyMain;

        uint8_t buffer[16] __attribute__((aligned(4))); //Our 16 byte buffer

        //Copy to our buffer the contents of the MRAM of the next 16 byte block
        memcpy(buffer, dst, 16);
        //Copy the new contents to the buffer
        memset(buffer ,value, size);

        //Program this last chunk
        result |= am_hal_mram_main_program(AM_HAL_MRAM_PROGRAM_KEY,(uint32_t*)buffer,(uint32_t*)dst,16/4);
    }

exit:;
    if(result!=0)
    {
        LOG_E("k_setToMRAM finished with a non 0 (%d) code!!!!!",result);
    }     
}



//Quick alignment testing code

/* __BKPT(1);

k_setToMRAM(k_appStorageMRAMRegion,0xA,1000);

uint8_t testBuffer[256];
for(int i=0;i<sizeof(testBuffer);i++) testBuffer[i]=i;

k_copyToMRAM((uint8_t*)k_appStorageMRAMRegion + 5,testBuffer+1,235);


uint8_t testBuffer2[512];
for(int i=0;i<sizeof(testBuffer2);i++) testBuffer2[i]= ((uint8_t*)k_appStorageMRAMRegion)[i];

LOG_I("Diff: %d",memcmp(testBuffer2,testBuffer,sizeof(testBuffer)));*/