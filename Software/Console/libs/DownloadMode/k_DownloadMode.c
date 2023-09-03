#include "DownloadMode.h"
#include "Misc.h"
#include "hal/mcu/am_hal_uart.h"
#include "string.h"
#include "stdio.h"
#include "Helpers/Flash.h"
#include "MainMenu.h"
#include "Kernel.h"
#include "TimeKeeper.h"
#include "Kernel_Draw.h"
#include "stdlib.h"
#include "math.h"
#include "Helpers/FileSystem.h"

uint8_t dataBuffer[4096];

extern void *phUART;

bool ProcessChunk(void* UARThandle, int baseAddressOffset);
uint8_t ReadNextByte(void* UARThandle, bool shortTimeout);
int16_t ReadNextInt16(void* UARThandle);
int32_t ReadNextInt32(void* UARThandle);
void ForceRead(void* buffer, void* UARThandle,int byteNumToReceive, bool shortTimeout);


int totalLengthWritten=0;

int currentBlockNumber=0;

void DownloadMode_DoWork(void* UARThandle){

    LOG_I("Entered flash download mode. Waiting for the PC app to continue...");
    while(!(ReadNextByte(UARThandle, true)==0x69)){ //Wait for the magic byte for the download
        //LOG_E("Failed to receive first magic download byte!");
        k_input_state state;
         k_GetInput(&state,false);
        if((state.triggerLeft && state.triggerRight && state.buttonPower)){
            k_setShouldCurrentAppExit(true);
            return;
        }
        
    }

    int startAddress = ReadNextInt32(UARThandle);
    LOG_I("Received starting offset: %d",startAddress);
    LOG_I("Magic byte received. Starting to process chunks until one with size 0 is found...");
    currentBlockNumber=0;
    totalLengthWritten=0;
    while(ProcessChunk(UARThandle, startAddress)){
        currentBlockNumber++;
    }

    LOG_I("Calculating final checksum...");
    uint32_t finalChecksum=CalculateChecksum((uint8_t*)MSPI_XIP_BASE_ADDRESS + startAddress,totalLengthWritten);
    LOG_I("UPLOAD|CHK|%d",finalChecksum);
    DelayMS(500);

    
    LOG_I("Flash download mode exited!");
}

bool ProcessChunk(void* UARThandle, int baseAddressOffset){
    int32_t length = ReadNextInt32(UARThandle);
    LOG_I("Got length: %d",length);
    if(length>sizeof(dataBuffer)) {
        LOG_E("Can not fit download chunk into buffer!");
        SpinLoop();
    }

    if(length==0) return false;

  //  LOG_E("Reading %d bytes...",length);
    ForceRead(dataBuffer,UARThandle,length,false);    

    uint32_t proposedChecksum=(uint32_t)ReadNextInt32(UARThandle);
  //  LOG_I("Remote checksum: %d",proposedChecksum);
 //   LOG_I("Local checksum: %d",CalculateChecksum(dataBuffer,length));

    if(proposedChecksum==CalculateChecksum(dataBuffer,length)){
    //    LOG_I("Checksum for chunk %d received successfully");
        totalLengthWritten+=length;

        Flash_DisableXIP();
       // LOG_I("Erasing...");
        Flash_Erase4KBBlock(baseAddressOffset + (currentBlockNumber<<12));
     //   LOG_I("Writing...");
        Flash_Write_InternalAddress(baseAddressOffset + (currentBlockNumber<<12),dataBuffer,length);

        Flash_EnableXIP();


        am_hal_cachectrl_control(AM_HAL_CACHECTRL_CONTROL_MRAM_CACHE_INVALIDATE, 0);       
     //   LOG_I("Checking...");        

      //  Flash_Print((currentBlockNumber<<12),length);
      //  Flash_Print(0,length);
        if(memcmp((void*)(MSPI_XIP_BASE_ADDRESS + baseAddressOffset + (currentBlockNumber<<12)),dataBuffer,length)==0){
            LOG_I("UPLOAD|OK");
        }else{
            LOG_I("Flash readback failed!");
            SpinLoop();
        }


        return true;
    }else{
        LOG_E("Received wrong checksum!");        
        SpinLoop();
    }

    return true;
}

uint8_t ReadNextByte(void* UARThandle, bool shortTimeout){
    uint8_t toReturn=0;
    ForceRead(&toReturn,UARThandle,1, shortTimeout);
    return toReturn;
}

int16_t ReadNextInt16(void* UARThandle){
    int16_t toReturn;    
    ForceRead(&toReturn,UARThandle,2,false);
    return toReturn;
}

int32_t ReadNextInt32(void* UARThandle){
    int32_t toReturn;    
    ForceRead(&toReturn,UARThandle,4,false);   
    return toReturn;
}


void ForceRead(void* buffer, void* UARThandle,int byteNumToReceive, bool shortTimeout){
    uint8_t* ptrCopy=buffer;
    int bytesReadTotal=0;

    while(bytesReadTotal<byteNumToReceive){
        uint32_t bytesReceivedOK =0;
        am_hal_uart_transfer_t config ={
            .eType=AM_HAL_UART_BLOCKING_READ,
            .pui8Data =buffer,
            .ui32NumBytes = byteNumToReceive,
            .ui32TimeoutMs= shortTimeout?100:2500,
            .pui32BytesTransferred=&bytesReceivedOK
        };
        uint32_t result = am_hal_uart_transfer(UARThandle,&config);
       //  am_hal_uart_fifo_read(UARThandle,ptrCopy,byteNumToReceive,&bytesReadLastTime);
        //if(bytesReadLastTime>0){
            //LOG_I("Read: %d",bytesReceivedOK);
        //}
        if(result!=0){
            LOG_I("Read error: %d",result);
            return;
        }
        bytesReadTotal+=bytesReceivedOK;
        ptrCopy+=bytesReceivedOK;      
    }  
}

uint32_t CalculateChecksum(void* buffer,int length){

    uint8_t* buffer8=buffer;
    uint8_t midBuffer[32]; //Use 16 byte since that is the size of the AXI buffer
    uint32_t toReturn=0;

    while(length>0){
        uint32_t amountToReadNow=MIN(length,sizeof(midBuffer));
        memcpy(midBuffer,buffer8,amountToReadNow);
        for(int i=0;i<amountToReadNow;i++){        
            toReturn+=midBuffer[i];
        }

        length-=amountToReadNow;
        buffer8+=amountToReadNow;
    }

    return toReturn;
}



void DownloadModeEntryPoint(){

    k_BeginScreenUpdate();
    k_draw_Clear(K_COLOR_WHITE);       
    k_image downloadIcon = GetImageByName("downloadIcon");
    k_draw_DrawImageGeneric(&downloadIcon,SCREEN_WIDTH_REAL/2 - downloadIcon.width, SCREEN_HEIGHT_REAL/2-downloadIcon.height-20,2);    
    k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,SCREEN_HEIGHT_REAL/2+60,2,"In download mode");
    k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,SCREEN_HEIGHT_REAL/2+75,1,"Please continue in the PC app");
    k_EndScreenUpdate(true,true);

    while(!k_getShouldCurrentAppExit()){
        DownloadMode_DoWork(phUART);
    }
}



CHECKPOINT_EXCLUDE_DATA uint8_t DownloadMode_Icon[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xc0, 0xff, 0xff, 0xfe, 0x1f, 0xe0, 0x3f, 0xc0, 0xff, 0xff, 0xfc, 0x0f, 0xe0, 0x1f, 
	0xc0, 0xff, 0xff, 0xf8, 0x07, 0xe0, 0x1f, 0xc0, 0xff, 0xff, 0xf0, 0x03, 0xe0, 0x1f, 0xc0, 0xff, 
	0xff, 0xc0, 0x00, 0xe0, 0x1f, 0xc0, 0xff, 0xff, 0x80, 0x00, 0x60, 0x1f, 0xc0, 0xff, 0xff, 0x00, 
	0xc0, 0x20, 0x1f, 0xc0, 0xff, 0xfe, 0x01, 0xf0, 0x00, 0x1f, 0xc0, 0xff, 0xfc, 0x07, 0xb8, 0x00, 
	0x1f, 0xc0, 0xff, 0xf0, 0x0e, 0x1c, 0x00, 0x1f, 0xc0, 0xff, 0xe0, 0x1c, 0x0e, 0x00, 0x1f, 0xc0, 
	0xff, 0xc0, 0x38, 0x07, 0x00, 0x1f, 0xc0, 0xff, 0x80, 0x70, 0x03, 0xc0, 0x1f, 0xc0, 0xff, 0x01, 
	0xe0, 0x01, 0xe0, 0x1f, 0xc0, 0xfc, 0x03, 0x80, 0x00, 0x70, 0x0f, 0xc0, 0xf8, 0x07, 0x00, 0x00, 
	0x38, 0x07, 0xc0, 0xf0, 0x0e, 0x00, 0x00, 0x1c, 0x03, 0xc0, 0xe0, 0x1c, 0x00, 0x00, 0x0e, 0x01, 
	0xc0, 0xc0, 0x70, 0x00, 0x00, 0x03, 0x80, 0xc0, 0x00, 0xe0, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x01, 
	0xc0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x83, 0x80, 0x00, 0x00, 0x00, 0x70, 0x40, 0xcf, 0x00, 0x00, 
	0x00, 0x00, 0x3c, 0xc0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x00, 0x00, 0x00, 
	0x1f, 0xc0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 
	0xfe, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 
	0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 
	0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 
	0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 
	0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 0xf8, 0x00, 0x1f, 0xc0, 0xfe, 0x00, 0x07, 
	0xf8, 0x00, 0x1f, 0xc0, 0xff, 0x00, 0x07, 0xf8, 0x00, 0x3f, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0
};