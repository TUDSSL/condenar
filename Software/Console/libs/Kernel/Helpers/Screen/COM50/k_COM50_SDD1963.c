#include "ScreenCOM50.h"
#include "PinDefinitions.h"
#include "Helpers/Misc.h"
#include "am_bsp_pins.h"
#include "am_bsp.h"
#include "SSD1963_defs.h"
#include "Screen_Base.h"
#include "string.h"
#include "Kernel.h"
#include "TimeKeeper.h"

#define SSD1963_MSPI_MODULE 0 //You also need to change the ISR method name
void* SSD1963_MSPIHandle;

uint32_t DMA_buffer[1024];

volatile bool asyncOperationDone = true;

am_hal_mspi_dev_config_t MSPI_SSD1963_Config =
{
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_1,
    .eClockFreq           = AM_HAL_MSPI_CLK_3MHZ,
    .ui8TurnAround        = 0,
    .eAddrCfg             = AM_HAL_MSPI_ADDR_1_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_OCTAL_CE1,
    .bSendInstr           = false,
    .bSendAddr            = false,
    .bTurnaround          = false,
};

am_hal_mspi_dev_config_t MSPI_SSD1963_ConfigAFTERSETUP =
{
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_1,
    .eClockFreq           = AM_HAL_MSPI_CLK_6MHZ,
    .ui8TurnAround        = 0,
    .eAddrCfg             = AM_HAL_MSPI_ADDR_1_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_OCTAL_CE1,
    .bSendInstr           = false,
    .bSendAddr            = false,
    .bTurnaround          = false,
    .ui16DMATimeLimit=65000,       // 100nsec units

    //! Maximum number of bytes DMA transaction is allowed before releasing CE.
    .eDMABoundary = AM_HAL_MSPI_BOUNDARY_NONE
 //  .ui16DMATimeLimit     = 1 * 10 //In 100ns units
};

void SSD1963_ChangeMSPISpeed(am_hal_mspi_clock_e newFreq){
    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_disable(SSD1963_MSPIHandle))
    {
        am_util_stdio_printf("Error - Failed to disable MSPI.\n");
        return;
    }  

    MSPI_SSD1963_Config.eClockFreq = newFreq;
    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_device_configure(SSD1963_MSPIHandle, &MSPI_SSD1963_Config))
    {
        am_util_stdio_printf("Error - Failed to configure MSPI.\n");
        return;
    }

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_enable(SSD1963_MSPIHandle))
    {
        am_util_stdio_printf("Error - Failed to enable MSPI.\n");
        return;
    }     
}

void SSD1963_Initialize(){
    GPIO_ModeIn(PIN_SSD1963_TE,false,false);
    GPIO_ModeOut(PIN_SSD1963_RW); //1=Read, 0=Write
    GPIO_SetLevel(PIN_SSD1963_RW,1); //Default read
    GPIO_ModeOut(PIN_SSD1963_DC); //1=Data, 0=Command
    GPIO_SetLevel(PIN_SSD1963_DC,1); //Default data
    GPIO_ModeOut(PIN_SSD1963_RESET);
    GPIO_SetLevel(PIN_SSD1963_RESET, 0);
    DelayMS(1);
    GPIO_SetLevel(PIN_SSD1963_RESET, 1);
    DelayMS(10);


    const am_hal_gpio_pincfg_t g_ADC_PIN_CFG = { .GP.cfg_b.uFuncSel = AM_HAL_PIN_53_NCE53, .GP.cfg_b.uNCE=33 };//The NCE number is undocumented, this one works for now
    am_hal_gpio_pinconfig(PIN_SSD1963_CS, g_ADC_PIN_CFG);
    
    const am_hal_gpio_pincfg_t g_CLK_PIN_CFG = { .GP.cfg_b.uFuncSel = AM_HAL_PIN_33_CLKOUT};
    am_hal_gpio_pinconfig(PIN_SSD1963_CLKIN, g_CLK_PIN_CFG);

    am_hal_clkgen_clkout_enable(true, CLKGEN_CLKOUT_CKSEL_HFRC2_24MHz); //Enable CLKOUT, this will be automatically changed after a frame by the Kernel to the appropiate value based on the config

    g_AM_BSP_GPIO_MSPI0_D0.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D1.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D2.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D3.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D4.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D5.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D6.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;
    g_AM_BSP_GPIO_MSPI0_D7.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;

        g_AM_BSP_GPIO_MSPI0_SCK.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X;

    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D0,  g_AM_BSP_GPIO_MSPI0_D0);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D1,  g_AM_BSP_GPIO_MSPI0_D1);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D2,  g_AM_BSP_GPIO_MSPI0_D2);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D3,  g_AM_BSP_GPIO_MSPI0_D3);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D4,  g_AM_BSP_GPIO_MSPI0_D4);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D5,  g_AM_BSP_GPIO_MSPI0_D5);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D6,  g_AM_BSP_GPIO_MSPI0_D6);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_D7,  g_AM_BSP_GPIO_MSPI0_D7);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI0_SCK, g_AM_BSP_GPIO_MSPI0_SCK);

   
    // Enable fault detection.
    am_hal_fault_capture_enable();

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_initialize(SSD1963_MSPI_MODULE, &SSD1963_MSPIHandle))
    {
        am_util_stdio_printf("Error - Failed to initialize MSPI.\n");
        return;
    }
    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_power_control(SSD1963_MSPIHandle, AM_HAL_SYSCTRL_WAKE, false))
    {
        am_util_stdio_printf("Error - Failed to power on MSPI.\n");
        return;
    }

    am_hal_mspi_config_t generalConfig = {
        .bClkonD4=false,
        .pTCB=DMA_buffer,
        .ui32TCBSize=(sizeof(DMA_buffer) / sizeof(uint32_t))
    };

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_configure(SSD1963_MSPIHandle, &generalConfig))
    {
        am_util_stdio_printf("Error - Failed to configure MSPI.\n");
        return;
    }

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_device_configure(SSD1963_MSPIHandle, &MSPI_SSD1963_Config))
    {
        am_util_stdio_printf("Error - Failed to configure MSPI.\n");
        return;
    }
    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_enable(SSD1963_MSPIHandle))
    {
        am_util_stdio_printf("Error - Failed to enable MSPI.\n");
        return;
    }  


    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_interrupt_clear(SSD1963_MSPIHandle, AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR ))
    {
        am_util_stdio_printf("Error - Failed to clear interrupts on MSPI.\n");
        return;
    }

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_interrupt_enable(SSD1963_MSPIHandle, AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR ))
    {
        am_util_stdio_printf("Error - Failed to enable interrupts on MSPI.\n");
        return;
    }
    
    #if SSD1963_MSPI_MODULE == 0
    NVIC_EnableIRQ(MSPI0_IRQn);
    #elif SSD1963_MSPI_MODULE == 1
    NVIC_EnableIRQ(MSPI1_IRQn);
    #elif SSD1963_MSPI_MODULE == 2
    NVIC_EnableIRQ(MSPI2_IRQn);
    #else
    #error "Invalid MSPI num"
    #endif
    
    am_hal_interrupt_master_enable();

    uint32_t deviceID = SSD1963_ReadRegister32(SSD1963_REG_READ_DDB);
    if(deviceID!=SSD1963_ID){
        LOG_E("Failed to identify SSD IC!");
        return;
    }else{
        LOG_I("Found SSD1963 IC");
    }

    bool usePll = false;

    //Configure PLL
    SSD1963_WriteRegister8(SSD1963_REG_SET_PLL, 0); //Disable PLL and PLL lock

    if(usePll){
        //6MHZ --> 100MHz: x16.666... (50/3) So M=49 and N=2
        uint8_t PLL_MN_buffer[3] = { 49, 2 | 0b00100000, 0b00000100 };
        SSD1963_WriteRegister(SSD1963_REG_SET_PLL_MN, PLL_MN_buffer, 3);

        SSD1963_WriteRegister8(SSD1963_REG_SET_PLL, 1); //Enable PLL
        DelayMS(1); //Wait for the PLL to stabilize
        SSD1963_WriteRegister8(SSD1963_REG_SET_PLL, 3); //Lock PLL
        DelayMS(1); //Wait for the PLL to lock
    }
  

  //  SSD1963_WriteRegister8(SSD1963_REG_SET_PLL, 0); //Disable PLL  

    SSD1963_WriteRegister(SSD1963_REG_SOFT_RESET, NULL, 0);
    DelayMS(10);




    //Set pixel clock freq. LSHIFT_CLK = (SYS_CLK/1,048,576) * (LCDC_FPR+1)
    //We want 5MHz output clock, so  LCDC_FPR = 50000;  If SYS_CLK=100MHz, LSHIFT_CLK=4.768MHz
    uint32_t lshiftSetting = 75000/2;
    if(!usePll){ //If we are not using the PLL, we need to do this to emulate it
        lshiftSetting = 0b11111111111111111111 / ((1 ));
    } 

    if(k_GetSettingBool("/Energy/High refresh rate", true)==false){
        lshiftSetting/=2;
    }     

    uint8_t LSHIFT_buffer[3] = {(lshiftSetting >> 16) & 0x0F, (lshiftSetting >> 8),lshiftSetting};
    SSD1963_WriteRegister(SSD1963_REG_SET_LSHIFT_FREQ, LSHIFT_buffer, 3);

    //Set LCD mode
    uint32_t lcdWidth = 320;
    uint32_t lcdHeight = 240;
    uint8_t LCDModeBuffer[7] = {
        0b00100000, //Last 3 bits are CLK, HSYNC and VSYNC polarities
        0b00000000, //[6:5] is the TFT type
        ((lcdWidth-1) >> 8) & 0x00000111,
        (lcdWidth-1),
        ((lcdHeight-1) >> 8) & 0x00000111,
        (lcdHeight-1),
        0 //Serial TFT RGB format. We are not using this (I think?)
    };
    SSD1963_WriteRegister(SSD1963_REG_SET_LCD_MODE_,LCDModeBuffer,7);

    //Set LCD timings (HSYNC)
    uint32_t lcdHSYNCPeriod = 429;
    uint32_t lcdHSYNCActiveTime = 11;
    uint32_t lcdHSYNCdataValidOffset = 74 + 3;
    uint8_t LCDTimingsBufferH[8] = {
        ((lcdHSYNCPeriod-1) >> 8) & 0x00000111, //HT, total hsync period
        (lcdHSYNCPeriod-1),
        ((lcdHSYNCdataValidOffset) >> 8) & 0x00000111, //HPS, offset from HSYNC to data start
        (lcdHSYNCdataValidOffset),
        (lcdHSYNCActiveTime-1), //HPW, width of the HSYNC pulse
        0, //LPS = 0, offset of the HSYNC pulse
        0, //LPS = 0
        0 //No idea what this is LPSPP[1:0]
    };
    SSD1963_WriteRegister(SSD1963_REG_SET_HORI_PERIOD,LCDTimingsBufferH,8);

    //Set LCD timings (VSYNC)
    uint32_t lcdVSYNCPeriod = 272;
    uint32_t lcdVSYNCActiveTime = 3;
    uint32_t lcdVSYNCdataValidOffset = 21;
    uint8_t LCDTimingsBufferV[7] = {
        ((lcdVSYNCPeriod-1) >> 8) & 0x00000111, //HT, total hsync period
        (lcdVSYNCPeriod-1),
        ((lcdVSYNCdataValidOffset) >> 8) & 0x00000111, //HPS, offset from HSYNC to data start
        (lcdVSYNCdataValidOffset),
        (lcdVSYNCActiveTime-1), //VPW, width of the VSYNC pulse
        0, //FPS = 0, offset of the VSYNC pulse
        0 //FPS = 0, offset of the VSYNC pulse
    };
    SSD1963_WriteRegister(SSD1963_REG_SET_VERT_PERIOD,LCDTimingsBufferV,7);

    SSD1963_WriteRegister8(SSD1963_REG_SET_PIXEL_DATA_INTERFACE, 0); //8bit bus width from the apollo

    DelayMS(10);

    SSD1963_WriteRegister8(SSD1963_REG_SET_ADDRESS_MODE, 0b00000011); //Rotate 180º (two inversions)

    SSD1963_WriteRegister(SSD1963_REG_SET_DISPLAY_ON, NULL, 0); //Turn on the display   

    //Set PWM data
    uint8_t PWMfreq = 0;
    uint32_t PWMduty = 16;
    uint8_t PWMsettings[6] = {
        PWMfreq, 
        PWMduty,
        0x00000001,
        0,
        0, 
        0
    };
    SSD1963_WriteRegister(SSD1963_REG_SET_PWM_CONF,PWMsettings,6);

    if (AM_HAL_STATUS_SUCCESS != am_hal_mspi_device_configure(SSD1963_MSPIHandle, &MSPI_SSD1963_ConfigAFTERSETUP))
    {
        am_util_stdio_printf("Error - Failed to configure MSPI.\n");
        return;
    }
}




void am_mspi0_isr(void)
{
    uint32_t ui32Status;
    if (!am_hal_mspi_interrupt_status_get(SSD1963_MSPIHandle, &ui32Status, false))
    {
        if ( ui32Status )
        {
            am_hal_mspi_interrupt_clear(SSD1963_MSPIHandle, ui32Status);
            am_hal_mspi_interrupt_service(SSD1963_MSPIHandle, ui32Status);
        }
    }    
}

void SSD1963_ReadRegister(uint8_t registerNum, uint8_t* buffer, uint32_t lenght){
    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 0); //Command
    am_hal_mspi_pio_transfer_t  transaction = {0};
    transaction.eDirection = AM_HAL_MSPI_TX;    
    transaction.ui32NumBytes = 1;
    transaction.pui32Buffer = (uint32_t*)&registerNum;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

    GPIO_SetLevel(PIN_SSD1963_RW, 1); //Read
    GPIO_SetLevel(PIN_SSD1963_DC, 1); //Data
    transaction.eDirection = AM_HAL_MSPI_RX;  
    transaction.ui32NumBytes = lenght;
    transaction.pui32Buffer = (uint32_t*)buffer;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);
}

void SSD1963_WriteRegister(uint8_t registerNum, uint8_t* buffer, uint32_t lenght){
    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 0); //Command
    am_hal_mspi_pio_transfer_t  transaction = {0};
    transaction.eDirection = AM_HAL_MSPI_TX;    
    transaction.ui32NumBytes = 1;
    transaction.pui32Buffer = (uint32_t*)&registerNum;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

    if(lenght>0){
        GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
        GPIO_SetLevel(PIN_SSD1963_DC, 1); //Data
        transaction.eDirection = AM_HAL_MSPI_TX;  
        transaction.ui32NumBytes = lenght;
        transaction.pui32Buffer = (uint32_t*)buffer;
        am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);
    }
}

void SSD1963_WriteRegisterMemSet(uint8_t registerNum, uint8_t valueToSet, uint32_t lenght){
    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 0); //Command
    am_hal_mspi_pio_transfer_t  transaction = {0};
    transaction.eDirection = AM_HAL_MSPI_TX;    
    transaction.ui32NumBytes = 1;
    transaction.pui32Buffer = (uint32_t*)&registerNum;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

    uint8_t buffer[64];
    memset(buffer,valueToSet, sizeof(buffer));

    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 1); //Data

    while(lenght>0){
        int bytesToWriteNow = MIN(sizeof(buffer),lenght);
        transaction.eDirection = AM_HAL_MSPI_TX;  
        transaction.ui32NumBytes = bytesToWriteNow;
        transaction.pui32Buffer = (uint32_t*)buffer;
        am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

        lenght -= bytesToWriteNow;
    }
}

void SSD1963_WriteRegisterColorSet(uint8_t registerNum, k_color colorToSet, uint32_t pixelNum){
    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 0); //Command
    am_hal_mspi_pio_transfer_t  transaction = {0};
    transaction.eDirection = AM_HAL_MSPI_TX;    
    transaction.ui32NumBytes = 1;
    transaction.pui32Buffer = (uint32_t*)&registerNum;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

    #define BUFFER_SIZE_PIXELS 32
    k_color buffer[BUFFER_SIZE_PIXELS];
    for(int i=0;i<sizeof(buffer)/sizeof(k_color);i++) buffer[i] = colorToSet;

    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 1); //Data

    while(pixelNum>0){
        int pixelsToWriteNow = MIN(BUFFER_SIZE_PIXELS,pixelNum);
        transaction.eDirection = AM_HAL_MSPI_TX;  
        transaction.ui32NumBytes = pixelsToWriteNow * 3;
        transaction.pui32Buffer = (uint32_t*)buffer;
        am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

        pixelNum -= pixelsToWriteNow;
    }
}

void SSD1963_WriteRegisterMemCpy(uint8_t registerNum, uint8_t* srcBuffer, uint32_t lenght){
    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 0); //Command
    am_hal_mspi_pio_transfer_t  transaction = {0};
    transaction.eDirection = AM_HAL_MSPI_TX;    
    transaction.ui32NumBytes = 1;
    transaction.pui32Buffer = (uint32_t*)&registerNum;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

    while(lenght>0){
        //32 * 4 - 1
        int bytesToWriteNow = MIN(24,lenght); //Limitations of the MSPI peripheral (max bytes of a single blocking transaction)

        GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
        GPIO_SetLevel(PIN_SSD1963_DC, 1); //Data
        transaction.eDirection = AM_HAL_MSPI_TX;  
        transaction.ui32NumBytes = bytesToWriteNow;
        transaction.pui32Buffer = (uint32_t*)srcBuffer;
        am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

        lenght -= bytesToWriteNow;
        srcBuffer+= bytesToWriteNow;
    }
}

void asyncCallback(void* arg, uint32_t status){   
    asyncOperationDone = true;
}

void SSD1963_WaitForAsync(){
    while(!asyncOperationDone) {
        k_lowPowerSleepUntil(millis()+1); //1ms
    };    
}

void SSD1963_WriteRegisterMemCpyAsync(uint8_t registerNum, uint8_t* srcBuffer, uint32_t lenght){

    asyncOperationDone = false;

    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 0); //Command
    am_hal_mspi_pio_transfer_t  transaction = {0};
    transaction.eDirection = AM_HAL_MSPI_TX;    
    transaction.ui32NumBytes = 1;
    transaction.pui32Buffer = (uint32_t*)&registerNum;
    am_hal_mspi_blocking_transfer(SSD1963_MSPIHandle, &transaction, 1000000);

    GPIO_SetLevel(PIN_SSD1963_RW, 0); //Write
    GPIO_SetLevel(PIN_SSD1963_DC, 1); //Data

    am_hal_mspi_dma_transfer_t dmaTransfer = {0};
    dmaTransfer.ui8Priority = 1;
    dmaTransfer.eDirection = AM_HAL_MSPI_TX;    
    dmaTransfer.ui32SRAMAddress = (uint32_t)srcBuffer;
    dmaTransfer.ui32TransferCount = lenght;
    uint32_t result = am_hal_mspi_nonblocking_transfer(SSD1963_MSPIHandle, &dmaTransfer, AM_HAL_MSPI_TRANS_DMA, asyncCallback, NULL);
}

uint8_t SSD1963_ReadRegister8(uint8_t registerNum){
    uint8_t toReturn;
    SSD1963_ReadRegister(registerNum, (uint8_t*)&toReturn, 1);
    return toReturn;
}

void SSD1963_WriteRegister8(uint8_t registerNum, uint8_t registerValue){
    SSD1963_WriteRegister(registerNum, (uint8_t*)&registerValue, 1);
}

uint32_t SSD1963_ReadRegister32(uint8_t registerNum){
    uint32_t toReturn;
    SSD1963_ReadRegister(registerNum, (uint8_t*)&toReturn, 4);
    return toReturn;
}

void SSD1963_WriteRegister32(uint8_t registerNum, uint32_t registerValue){
    SSD1963_WriteRegister(registerNum, (uint8_t*)&registerValue, 4);
}

void SSD1963_SetupDrawRegion(uint32_t startX, uint32_t stopX, uint32_t startY, uint32_t stopY){
    uint8_t colData[4] = { startX >> 8, startX, stopX >> 8, stopX };
    SSD1963_WriteRegister(SSD1963_REG_SET_COLUMN_ADDRESS, colData, 4);
    uint8_t rowData[4] = { startY >> 8, startY, stopY >> 8, stopY };
    SSD1963_WriteRegister(SSD1963_REG_SET_PAGE_ADDRESS, rowData, 4);
}

void SSD1963_SetToDrawRegion(bool resetStartingPoint, k_color colorToSet, uint32_t pixelsToSet){
    SSD1963_WriteRegisterColorSet(resetStartingPoint ? SSD1963_REG_WRITE_MEMORY_START : SSD1963_REG_WRITE_MEMORY_CONTINUE, colorToSet, pixelsToSet);
}

void SSD1963_SendToDrawRegion(bool resetStartingPoint, k_color* dataToSend, uint32_t pixelsToSend){
    SSD1963_WriteRegisterMemCpy(resetStartingPoint ? SSD1963_REG_WRITE_MEMORY_START : SSD1963_REG_WRITE_MEMORY_CONTINUE, (uint8_t*)dataToSend, pixelsToSend * 3);
}

void SSD1963_SendToDrawRegionAsync(bool resetStartingPoint, k_color* dataToSend, uint32_t pixelsToSend){
    SSD1963_WriteRegisterMemCpyAsync(resetStartingPoint ? SSD1963_REG_WRITE_MEMORY_START : SSD1963_REG_WRITE_MEMORY_CONTINUE, (uint8_t*)dataToSend, pixelsToSend * 3);
}