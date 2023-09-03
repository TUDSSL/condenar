#include "Misc.h"
#include "Kernel.h"

#include "Screen.h"
#include "Misc.h"
#include "PinDefinitions.h"
#include "am_mcu_apollo.h"
#include "Misc.h"
#include <string.h>
#include "SEGGER_RTT.h"
#include "Kernel.h"
#include "Kernel_Draw.h"
#include "am_bsp.h"
#include "am_bsp_pins.h"

#define OPT3001_REG_RESULT 0x00
#define OPT3001_REG_CONFIG 0x01
#define OPT3001_REG_LOWLIMIT 0x02
#define OPT3001_REG_HIGHLIMIT 0x03
#define OPT3001_REG_MANID 0x7E
#define OPT3001_REG_DEVID 0x7F

#define OPT3001_MANID 0x4954
#define OPT3001_DEVID 0x0130

#define LIGHTSENSOR_IOM_NUM 6 //You also need to check the ISR method name and the enable IRQ method
#define LIGHTSENSOR_ADDR 0x44

void* lightSensor_iomHandle;

bool LightSensor_ReadRegister(uint8_t registerNum, uint8_t* data);
bool LightSensor_WriteRegister(uint8_t registerNum, uint8_t* data);


void am_iomaster6_isr(void)
{
    uint32_t ui32Status;

    if (!am_hal_iom_interrupt_status_get(lightSensor_iomHandle, true, &ui32Status))
    {
        if ( ui32Status )
        {
            am_hal_iom_interrupt_clear(lightSensor_iomHandle, ui32Status);
            am_hal_iom_interrupt_service(lightSensor_iomHandle, ui32Status);
        }
    }
}

uint8_t lightSensor_DMAbuffer[64];


void LightSensor_Initialize(){
    
    LOG_I("Initializing light sensor...");
    am_hal_iom_initialize(LIGHTSENSOR_IOM_NUM,&lightSensor_iomHandle);    
    am_hal_iom_power_ctrl(lightSensor_iomHandle, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_iom_config_t g_sIOMSpiConfig =
    {
        .eInterfaceMode = AM_HAL_IOM_I2C_MODE,
        .ui32ClockFreq = AM_HAL_IOM_400KHZ,
        .pNBTxnBuf=(uint32_t*)lightSensor_DMAbuffer,
        .ui32NBTxnBufLength=sizeof(lightSensor_DMAbuffer)/4
    };
    am_hal_iom_configure(lightSensor_iomHandle, &g_sIOMSpiConfig);

    am_hal_gpio_pincfg_t SCLsettings = {
        .GP.cfg_b.uFuncSel            = AM_HAL_PIN_61_M6SCL,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    };
    am_hal_gpio_pincfg_t SDAsettings = {
        .GP.cfg_b.uFuncSel           = AM_HAL_PIN_62_M6SDAWIR3,
        .GP.cfg_b.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA
    };
    

    am_hal_interrupt_master_enable();
    am_hal_iom_interrupt_enable(lightSensor_iomHandle, 0xFF);
    NVIC_EnableIRQ(IOMSTR6_IRQn);

    am_hal_gpio_pinconfig(PIN_LIGHTSENSOR_SDA, SDAsettings);
    am_hal_gpio_pinconfig(PIN_LIGHTSENSOR_SCL, SCLsettings);

    am_hal_iom_enable(lightSensor_iomHandle);

    bool success = true;
    uint16_t manId = 0, devId = 0;
    success = success && LightSensor_ReadRegister(OPT3001_REG_MANID, &manId);
    success = success && LightSensor_ReadRegister(OPT3001_REG_DEVID, &devId);
    success = success && manId == OPT3001_MANID;
    success = success && devId == OPT3001_DEVID;

    uint8_t configBytes[2] = {0b11000110, 0b00000000};
    success = success && LightSensor_WriteRegister(OPT3001_REG_CONFIG, configBytes);

    uint16_t configBack = 0;
    success = success && LightSensor_ReadRegister(OPT3001_REG_CONFIG, &configBack);
    success = success && configBack == (uint16_t)(configBytes[1] << 8 | configBytes[0]);

    if(!success){
        LOG_E("Light sensor not found");
    }else{
        LOG_I("Light sensor found");
    }
}

bool LightSensor_ReadRegister(uint8_t registerNum, uint8_t* data){

    am_hal_iom_transfer_t       Transaction;
    Transaction.ui32InstrLen        = 1;
    Transaction.ui64Instr           = registerNum;
    Transaction.eDirection          = AM_HAL_IOM_RX;
    Transaction.ui32NumBytes        = 2;
    Transaction.pui32RxBuffer       = data;
    Transaction.bContinue           = false;
    Transaction.ui8RepeatCount      = 0;
    Transaction.ui32PauseCondition  = 0;
    Transaction.ui32StatusSetClr    = 0;

    Transaction.uPeerInfo.ui32I2CDevAddr = LIGHTSENSOR_ADDR;
    
    return am_hal_iom_blocking_transfer(lightSensor_iomHandle, &Transaction) == AM_HAL_STATUS_SUCCESS;
}


bool LightSensor_WriteRegister(uint8_t registerNum, uint8_t* data){

    am_hal_iom_transfer_t       Transaction;
    Transaction.ui32InstrLen        = 1;
    Transaction.ui64Instr           = registerNum;
    Transaction.eDirection          = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes        = 2;
    Transaction.pui32TxBuffer       = data;
    Transaction.bContinue           = false;
    Transaction.ui8RepeatCount      = 0;
    Transaction.ui32PauseCondition  = 0;
    Transaction.ui32StatusSetClr    = 0;

    Transaction.uPeerInfo.ui32I2CDevAddr = LIGHTSENSOR_ADDR;
    
    return am_hal_iom_blocking_transfer(lightSensor_iomHandle, &Transaction) == AM_HAL_STATUS_SUCCESS;
}

float LightSensor_GetValue(){
    uint16_t result = 0;
    bool success = LightSensor_ReadRegister(0, &result);
    if(!success){
        LOG_E("Failed to read light sensor value");
    }
    result = __REV16(result);
    uint16_t exponent = (result >> 12) & 0b1111;
    uint16_t mantissa = result & 0b111111111111;
    float computed = mantissa * 0.01f * pow(2, exponent);    
    return computed; 
}

