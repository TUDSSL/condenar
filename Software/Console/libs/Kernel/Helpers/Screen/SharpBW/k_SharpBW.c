#include "Screen/ScreenConfig.h"
#ifdef USE_SHARP_BW

#include "Screen.h"
#include "Misc.h"
#include "PinDefinitions.h"
#include "am_mcu_apollo.h"
#include "Misc.h"
#include <string.h>
#include "RNG.h"
#include "SEGGER_RTT.h"
#include "font8x8.h"
#include "Kernel.h"
#include "TimeKeeper.h"
#include "Kernel_Draw.h"
#include "am_bsp.h"
#include "am_bsp_pins.h"


void k_DoEndOfFrameStuff(bool checkpointsAllowed);
void DRAW_TUDScreen();
void Screen_SendUpdateAsync();

#define SCREEN_IOM_NUM 1 //You also need to check the ISR method name
void displayPrintf(int x, int y, const char *format, ...);
void SendTestScreen();


//From https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-bytes
uint8_t reverse(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


void k_EndScreenUpdate(bool checkpointsAllowed, bool showOverlays){
    if(showOverlays) k_DrawOverlays();
    Screen_SendUpdateAsync();
    k_DoEndOfFrameStuff(checkpointsAllowed);
}


struct SHARP_LINE_PACKET_BUFFER{
    union MODE
    {
        struct MODE_BYTE{
            uint8_t updateData:1;
            uint8_t invertFrame:1;
            uint8_t clearAll:1;
            uint8_t dummy:5;
        } modeSelect;
        uint8_t mode;
    }modeByte;
    
    
    uint8_t lineAddress;
    uint8_t pixelData[40];
    uint16_t dummyCycles;
} linePacketBuffer;


uint8_t DMA_buffer[1024];
uint8_t screenFinalBuffer[(320*240)/8];

void* screen_iomHandle;

volatile bool isSendingComplete=false;

void SendLine(uint8_t lineNum, uint8_t* lineData);


void am_iomaster0_isr(void)
{
    uint32_t ui32Status;

    if (!am_hal_iom_interrupt_status_get(screen_iomHandle, true, &ui32Status))
    {
        if ( ui32Status )
        {
            am_hal_iom_interrupt_clear(screen_iomHandle, ui32Status);
            am_hal_iom_interrupt_service(screen_iomHandle, ui32Status);
        }
    }
}

void SendNextAsyncPart();
void AsyncCallback(void* ptr, uint32_t transactionStatus);
volatile bool asyncDone=true;
int nextAsyncLine=0;

void DRAW_ClearScreen(bool black){
    memset(screenFinalBuffer,black?0x00:0xFF,sizeof(screenFinalBuffer));
}   

void DRAW_TUDScreen(){
    DRAW_ClearScreen(false);
    int offsetY=35;
    memcpy(screenFinalBuffer + (40*offsetY),epd_bitmap_TUD,40*145);   

    displayPrintf(10,200,"Hello this is a text iwth numbers: %d",69);
}

void Screen_IMPL_Initialize(){

    memset(&linePacketBuffer,0x69,sizeof(linePacketBuffer));
    LOG_I("Initializing screen...");
    am_hal_iom_initialize(SCREEN_IOM_NUM,&screen_iomHandle);
    am_hal_iom_power_ctrl(screen_iomHandle, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_iom_config_t g_sIOMSpiConfig =
    {
        .eInterfaceMode = AM_HAL_IOM_SPI_MODE,
        .ui32ClockFreq = AM_HAL_IOM_2MHZ,
        .eSpiMode = AM_HAL_IOM_SPI_MODE_0,
        .pNBTxnBuf=(uint32_t*)DMA_buffer,
        .ui32NBTxnBufLength=sizeof(DMA_buffer)/4
    };
    am_hal_iom_configure(screen_iomHandle, &g_sIOMSpiConfig);

    am_hal_gpio_pincfg_t DATAsettings = {
        .GP.cfg_b.uFuncSel            = AM_HAL_PIN_6_M0MOSI,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA
    };
    am_hal_gpio_pincfg_t CLKsettings = {
        .GP.cfg_b.uFuncSel           = AM_HAL_PIN_5_M0SCK,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA
    };
    am_hal_gpio_pincfg_t CSsettings =
    {
        .GP.cfg_b.uFuncSel            = AM_HAL_PIN_7_NCE7,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
        .GP.cfg_b.eGPOutCfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
        .GP.cfg_b.eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
        .GP.cfg_b.eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
        .GP.cfg_b.uNCE                = 0,
        .GP.cfg_b.eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH
    };

    am_hal_interrupt_master_enable();
    am_hal_iom_interrupt_enable(screen_iomHandle, 0xFF);
    NVIC_EnableIRQ(IOMSTR0_IRQn);

   // GPIO_ModeOut(PIN_SCREEN_CS);
   // GPIO_SetLevel(PIN_SCREEN_CS,0);
    am_hal_gpio_pinconfig(PIN_SCREEN_DT,   DATAsettings);
    am_hal_gpio_pinconfig(PIN_SCREEN_CLK,   CLKsettings);
    am_hal_gpio_pinconfig(PIN_SCREEN_CS,   CSsettings);
    GPIO_ModeOut(PIN_SCREEN_EXTCOMM);    

    am_hal_iom_enable(screen_iomHandle);
  
    //DRAW_TUDScreen();
    //Screen_SendUpdate();
    
    LOG_I("Screen initalized");
}



void Screen_SendUpdate(){

    Screen_SendUpdateAsync();

    Screen_WaitForFrameDone();
   // LOG_I("Updating screen...");
  /*  for(int i=1;i<=240;i++){
        SendLine(i, screenFinalBuffer+((320/8)*(i-1)));
    }

    GPIO_ToggleLevel(PIN_SCREEN_EXTCOMM);*/
}

void Screen_WaitForFrameDone(){
    while(!asyncDone) {};
}




void k_BeginScreenUpdate(){
    Screen_WaitForFrameDone(); //Wait for the screen to have been sent, to avoid overwriting the buffer

    #ifdef SEND_SCREEN_OVER_RTT
    if(k_GetSettingBool("/Debugging/Send screen over RTT", false)){
        while (SEGGER_RTT_GetBytesInBuffer(0)!=0) {}; //Wait for the screen to be send to the PC
    }        
    #endif
}





#pragma GCC optimize ("Ofast") 
void k_SetRawBufferByte(uint32_t index, uint8_t value){
    screenFinalBuffer[index]=value;
}


void SendLine(uint8_t lineNum, uint8_t* lineData){
    
    linePacketBuffer.modeByte.modeSelect.clearAll=0;
    linePacketBuffer.modeByte.modeSelect.invertFrame=0;
    linePacketBuffer.modeByte.modeSelect.updateData=1;
    linePacketBuffer.modeByte.modeSelect.dummy=0;
    linePacketBuffer.modeByte.mode=0x80;
    linePacketBuffer.dummyCycles=0;
    linePacketBuffer.lineAddress = reverse(lineNum);
    memcpy(linePacketBuffer.pixelData,lineData,sizeof(linePacketBuffer.pixelData));
  
    am_hal_iom_transfer_t Transaction;
    Transaction.ui32InstrLen    = 0;
    Transaction.ui64Instr       = 0;
    Transaction.eDirection      = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes    = sizeof(linePacketBuffer);
    Transaction.pui32TxBuffer   = (uint32_t*)(&linePacketBuffer);
    Transaction.bContinue       = false;
    Transaction.ui8RepeatCount  = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.ui8Priority     = 1;   
    Transaction.bContinue =false;
    Transaction.uPeerInfo.ui32SpiChipSelect = 0;      

    //GPIO_SetLevel(PIN_SCREEN_CS,1);
    am_hal_iom_blocking_transfer(screen_iomHandle,&Transaction);      
   // GPIO_SetLevel(PIN_SCREEN_CS,0); 
}

//Fast bit set from https://stackoverflow.com/questions/47981/how-do-i-set-clear-and-toggle-a-single-bit
void DRAW_SetPixel(int x, int y, k_color colorToDraw){
    bool isColorWhite = colorToDraw == K_COLOR_WHITE;
    int byteX = x/8;
    int remainder = x%8;
    uint8_t screenByte = screenFinalBuffer[y*40 + byteX];

    screenByte = (screenByte & ~(1 << (7-remainder))) | (isColorWhite << (7-remainder));

    screenFinalBuffer[y*40 + byteX] = screenByte;
}


void Screen_SendUpdateAsync(){
    asyncDone=false;
    nextAsyncLine=0;

    uint8_t header[] = {0x69,0x69,0x06,0x09,0x19,0x13,0x69,0x68};

    #ifdef SEND_SCREEN_OVER_RTT
    if(k_GetSettingBool("/Debugging/Send screen over RTT", false)){
        SEGGER_RTT_Write(0,header,sizeof(header));
        SEGGER_RTT_Write(0,screenFinalBuffer,sizeof(screenFinalBuffer));
    }           
    #endif

    SendNextAsyncPart();
}

void AsyncCallback(void* ptr,uint32_t transactionStatus){
    if(nextAsyncLine==SCREEN_HEIGHT_REAL){
        asyncDone=true;
    }else{
        SendNextAsyncPart();
        nextAsyncLine++;
    }
}

void SendNextAsyncPart(){
    linePacketBuffer.modeByte.modeSelect.clearAll=0;
    linePacketBuffer.modeByte.modeSelect.invertFrame=0;
    linePacketBuffer.modeByte.modeSelect.updateData=1;
    linePacketBuffer.modeByte.modeSelect.dummy=0;
    linePacketBuffer.modeByte.mode=0x80;
    linePacketBuffer.dummyCycles=0;
    linePacketBuffer.lineAddress = reverse(nextAsyncLine+1);
    memcpy(linePacketBuffer.pixelData,screenFinalBuffer+((320/8)*(nextAsyncLine)),sizeof(linePacketBuffer.pixelData));
  
    am_hal_iom_transfer_t Transaction;
    Transaction.ui32InstrLen    = 0;
    Transaction.ui64Instr       = 0;
    Transaction.eDirection      = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes    = sizeof(linePacketBuffer);
    Transaction.pui32TxBuffer   = (uint32_t*)(&linePacketBuffer);
    Transaction.bContinue       = false;
    Transaction.ui8RepeatCount  = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.ui8Priority     = 1;   
    Transaction.bContinue =false;
    Transaction.uPeerInfo.ui32SpiChipSelect = 0;      

   // GPIO_SetLevel(PIN_SCREEN_CS,1);
    am_hal_iom_nonblocking_transfer(screen_iomHandle,&Transaction,AsyncCallback,NULL);     
   // GPIO_SetLevel(PIN_SCREEN_CS,0); 
}

#endif