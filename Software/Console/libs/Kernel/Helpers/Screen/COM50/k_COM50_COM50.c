
#include "Helpers/Misc.h"
#include "ScreenCOM50.h"
#include "PinDefinitions.h"


#define SCREEN_IOM_NUM 5 //You also need to change the ISR method name

void* COM50_iomHandle;

uint32_t COM50_DMA_buffer[16];

void COM50_Initialize(){    
    am_hal_iom_initialize(SCREEN_IOM_NUM,&COM50_iomHandle);
    am_hal_iom_power_ctrl(COM50_iomHandle, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_iom_config_t g_sIOMSpiConfig =
    {
        .eInterfaceMode = AM_HAL_IOM_SPI_MODE,
        .ui32ClockFreq = AM_HAL_IOM_1MHZ,
        .eSpiMode = AM_HAL_IOM_SPI_MODE_0,
        .pNBTxnBuf=(uint32_t*)COM50_DMA_buffer,
        .ui32NBTxnBufLength=sizeof(COM50_DMA_buffer)/4
    };
    am_hal_iom_configure(COM50_iomHandle, &g_sIOMSpiConfig);

    am_hal_gpio_pincfg_t DATAsettings = {
        .GP.cfg_b.uFuncSel            = AM_HAL_PIN_48_M5MOSI,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA
    };
    am_hal_gpio_pincfg_t CLKsettings = {
        .GP.cfg_b.uFuncSel           = AM_HAL_PIN_47_M5SCK,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA
    };
    am_hal_gpio_pincfg_t CSsettings =
    {
        .GP.cfg_b.uFuncSel            = AM_HAL_PIN_22_NCE22,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
        .GP.cfg_b.eGPOutCfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
        .GP.cfg_b.eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
        .GP.cfg_b.eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
        .GP.cfg_b.uNCE                = 20, //4
        .GP.cfg_b.eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
    };

    am_hal_interrupt_master_enable();
    am_hal_iom_interrupt_enable(COM50_iomHandle, 0xFF);
    NVIC_EnableIRQ(IOMSTR1_IRQn);

    am_hal_gpio_pinconfig(PIN_COM50_DI,   DATAsettings);
    am_hal_gpio_pinconfig(PIN_COM50_CLK,   CLKsettings);
    am_hal_gpio_pinconfig(PIN_COM50_CS,   CSsettings);
    am_hal_iom_enable(COM50_iomHandle);

    
    COM50_SetBrightness(40); //0-63
    COM50_SetContrast(7); //0-15

    COM50_SetPowerMode(true);
}

void am_iomaster5_isr(void)
{
    uint32_t ui32Status;

    if (!am_hal_iom_interrupt_status_get(COM50_iomHandle, true, &ui32Status))
    {
        if ( ui32Status )
        {
            am_hal_iom_interrupt_clear(COM50_iomHandle, ui32Status);
            am_hal_iom_interrupt_service(COM50_iomHandle, ui32Status);
        }
    }
}


void COM50_WriteRegister8(uint8_t registerNum, uint8_t registerValue){
    uint8_t sendBuffer[2];
    sendBuffer[0] = reverseByte(registerNum) >> 4;
    sendBuffer[1] = reverseByte(registerValue);

    am_hal_iom_transfer_t Transaction;
    Transaction.ui32InstrLen    = 0;
    Transaction.ui64Instr       = 0;
    Transaction.eDirection      = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes    = 2;
    Transaction.pui32TxBuffer   = (uint32_t*)(sendBuffer);
    Transaction.bContinue       = false;
    Transaction.ui8RepeatCount  = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.ui8Priority     = 1;   
    Transaction.bContinue =false;
    Transaction.uPeerInfo.ui32SpiChipSelect = 0;      

    am_hal_iom_blocking_transfer(COM50_iomHandle,&Transaction);     
}


void COM50_SetBrightness(uint8_t brightness){
    COM50_WriteRegister8(0b00000000,brightness); //Change brightness   
}

void COM50_SetContrast(uint8_t contrast){
    COM50_WriteRegister8(0b00000010,contrast >> 4); //Change brightness   
}

void COM50_SetPowerMode(bool activeMode){
    uint8_t toSend = 0;
    if(activeMode) toSend |= 0b00010000;

    COM50_WriteRegister8(0b00000110,toSend);
}