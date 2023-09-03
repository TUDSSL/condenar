#include "Misc.h"
#include "Kernel.h"
#include "PinDefinitions.h"
#include "TimeKeeper.h"

volatile uint32_t lastCrankTimeEdge = 0;
volatile bool waitingForCrankRise = false;
volatile uint32_t lastCrankTimeUS = 0;
volatile uint32_t lastCrankTS = 0;
float lastCrankRPM = 0; //To average the RPM over time

 am_hal_gpio_pincfg_t crank_pin_config =
    {
        .GP.cfg_b.uFuncSel             = AM_HAL_PIN_18_GPIO,
        .GP.cfg_b.eGPInput             = AM_HAL_GPIO_PIN_INPUT_ENABLE,
        .GP.cfg_b.eGPRdZero            = AM_HAL_GPIO_PIN_RDZERO_READPIN,
        .GP.cfg_b.eIntDir              = AM_HAL_GPIO_PIN_INTDIR_HI2LO,
        .GP.cfg_b.eGPOutCfg            = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
        .GP.cfg_b.eDriveStrength       = AM_HAL_GPIO_PIN_DRIVESTRENGTH_0P1X,
        .GP.cfg_b.uSlewRate            = 0,
        .GP.cfg_b.ePullup              = AM_HAL_GPIO_PIN_PULLUP_NONE,
        .GP.cfg_b.uNCE                 = 0,
        .GP.cfg_b.eCEpol               = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW,
        .GP.cfg_b.uRsvd_0              = 0,
        .GP.cfg_b.ePowerSw             = AM_HAL_GPIO_PIN_POWERSW_NONE,
        .GP.cfg_b.eForceInputEn        = AM_HAL_GPIO_PIN_FORCEEN_NONE,
        .GP.cfg_b.eForceOutputEn       = AM_HAL_GPIO_PIN_FORCEEN_NONE,
        .GP.cfg_b.uRsvd_1              = 0,
    };

void am_gpio0_607f_isr(void){
    uint32_t ui32IntStatus;

    am_hal_gpio_interrupt_irq_status_get(GPIO0_607F_IRQn, true, &ui32IntStatus);
    am_hal_gpio_interrupt_irq_clear(GPIO0_607F_IRQn, ui32IntStatus);

    bool currentLevel = GPIO_GetLevel(PIN_SENSE_CRANK);
    //LOG_I("%d", currentLevel);
    if(currentLevel){
        if(waitingForCrankRise){
            waitingForCrankRise = false;
            lastCrankTimeUS = micros() - lastCrankTimeEdge;
            lastCrankTS = millis();
        }
    }else{
        lastCrankTimeEdge = micros();
        waitingForCrankRise = true;       
    }

    crank_pin_config.GP.cfg_b.eIntDir = currentLevel ? AM_HAL_GPIO_PIN_INTDIR_HI2LO : AM_HAL_GPIO_PIN_INTDIR_LO2HI;
    am_hal_gpio_pinconfig(PIN_SENSE_CRANK, crank_pin_config);
}

void k_CrankInitialize(){

    GPIO_ModeIn(PIN_SENSE_CRANK,false,false);   
    am_hal_gpio_pinconfig(PIN_SENSE_CRANK, crank_pin_config);

    int crankPin = PIN_SENSE_CRANK;
    am_hal_gpio_interrupt_control(AM_HAL_GPIO_INT_CHANNEL_0, AM_HAL_GPIO_INT_CTRL_INDV_ENABLE, (void *)&crankPin);
    NVIC_EnableIRQ((IRQn_Type)(GPIO0_607F_IRQn));
    
    LOG_I("Crank initialized"); 
}

float k_GetCrank(){
    if(lastCrankTimeUS == 0)
        return 0;
    if(millis() - lastCrankTS >500){
        lastCrankTimeUS = 0;
        return 0;
    }


    float k = 0.09f;
    float newVal = ((int32_t)lastCrankTimeUS - 1700)/50000.0f;
    if(newVal < 0)
        newVal = 0;
    if(newVal > 1)
        newVal = 1;

    lastCrankRPM = k * newVal + (1-k) * lastCrankRPM;
    return lastCrankRPM;
}