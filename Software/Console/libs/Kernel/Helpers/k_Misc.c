#include "Misc.h"
#include "hal/am_hal_gpio.h"
#include "hal/am_hal_timer.h"
#include "PinDefinitions.h"

static void *g_ADCHandle;
float g_fTrims[4];
volatile float lastADCsample[3];
volatile bool ADCsampleDone[3];

//When charged at 5V
//2.73
//3.81


int flashReads=0;

#define JOYSTICK_DEADBAND 0.037f
#define JOYSTICK_MINVAL 0.750f
#define JOYSTICK_MID 0.900f
#define JOYSTICK_MAXVAL 1.050f

void InitADC();
void ChangeBacklightPWMValue(uint8_t value);
void SetupBacklightPWM(int pin, uint8_t initialValue);

void GPIO_ModeOut(int pin){
    am_hal_gpio_pincfg_t pinConfig = {0};     
    pinConfig.GP.cfg_b.uFuncSel = 3; // Set GPIO function
    pinConfig.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
    pinConfig.GP.cfg_b.eGPOutCfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
    pinConfig.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA;
    am_hal_gpio_pinconfig(pin, pinConfig);
}

void GPIO_ModeIn(int pin,bool pullUpEnabled,bool pullDownEnabled){
    am_hal_gpio_pincfg_t pinConfig = {0};     
    pinConfig.GP.cfg_b.uFuncSel = 3; // Set GPIO function
    pinConfig.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
    if(pullUpEnabled) pinConfig.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLUP_12K;
    if(pullDownEnabled) pinConfig.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLDOWN_50K;
    pinConfig.GP.cfg_b.eGPInput  = AM_HAL_GPIO_PIN_INPUT_ENABLE;
    pinConfig.GP.cfg_b.eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN;
    am_hal_gpio_pinconfig(pin, pinConfig);
}

bool GPIO_GetLevel(int pin){
    uint32_t inputStatus=0;
    am_hal_gpio_state_read(pin, AM_HAL_GPIO_INPUT_READ, &inputStatus);
    return inputStatus==1;
}

void GPIO_SetLevel(int pin, bool value){
    am_hal_gpio_state_write(pin, value);
}

void GPIO_ToggleLevel(int pin){
    am_hal_gpio_output_toggle(pin);
}

void DelayMS(int ms){
    am_util_delay_ms(ms);
}

void nope(){
    
}

void SpinLoop(){
    LOG_W("Spin looping...");
    while (true) {}    
}


void SetupIO(){
    GPIO_ModeOut(PIN_FORCE_1V8_EN);
    GPIO_SetLevel(PIN_FORCE_1V8_EN,true); //Request 1V8 power to keep the apollo alive

    GPIO_ModeIn(PIN_BUTTON1,false,false);
    GPIO_ModeIn(PIN_BUTTON2,false,false);
    GPIO_ModeIn(PIN_BUTTON3,false,false);
    GPIO_ModeIn(PIN_BUTTON4,false,false);
    GPIO_ModeIn(PIN_BUTTON5,false,false);
    GPIO_ModeIn(PIN_BUTTON6,false,false);
    GPIO_ModeIn(PIN_BUTTON7,false,false);
    GPIO_ModeIn(PIN_BUTTON_POWER,false,false);

    GPIO_ModeIn(PIN_SENSE_CRANK,false,false);
    GPIO_ModeIn(PIN_SENSE_SOLAR,false,false);
    
    GPIO_ModeOut(PIN_UART_LOG);
    GPIO_SetLevel(PIN_UART_LOG, !1);

    GPIO_ModeOut(PIN_JOY_ON);
    GPIO_SetLevel(PIN_JOY_ON,false);

    GPIO_ModeOut(PIN_3V3_EN_LA);
    GPIO_SetLevel(PIN_3V3_EN_LA,1);

    GPIO_ModeOut(PIN_3V3_EN_AUTOON_LA);   

    GPIO_ModeOut(PIN_LATCH_STORE);

    SetAutoEnable(false);

    GPIO_ModeOut(PIN_ENABLE_MEM_VCC);
    GPIO_SetLevel(PIN_ENABLE_MEM_VCC,1);

    GPIO_ModeOut(PIN_ENABLE_AUX_1V8_VCC);
    GPIO_SetLevel(PIN_ENABLE_AUX_1V8_VCC,1);

    GPIO_ModeOut(PIN_ENABLE_AUX_VCC);
    GPIO_SetLevel(PIN_ENABLE_AUX_VCC,1);

    GPIO_ModeOut(60);
    GPIO_SetLevel(60,0);

    InitADC();

    GPIO_ModeOut(PIN_BL_DIM);
    GPIO_SetLevel(PIN_BL_DIM,1);
    SetupBacklightPWM(PIN_BL_DIM, 0); //No backlight by default
}

void SetAutoEnable(bool enable){
    GPIO_SetLevel(PIN_3V3_EN_AUTOON_LA,!enable);
    GPIO_SetLevel(PIN_LATCH_STORE,0);
    DelayMS(1);
    GPIO_SetLevel(PIN_LATCH_STORE,1);
    DelayMS(1);
    GPIO_SetLevel(PIN_LATCH_STORE,0);
}

void PowerEverythingDown(bool autoEnable){
    SetAutoEnable(autoEnable);
    DelayMS(100);
    GPIO_SetLevel(PIN_ENABLE_MEM_VCC,0);
    GPIO_SetLevel(PIN_ENABLE_AUX_1V8_VCC,0);
    GPIO_SetLevel(PIN_ENABLE_AUX_VCC,0);
    GPIO_SetLevel(PIN_3V3_EN_LA,0);
    ChangeBacklightPWMValue(0);    
    GPIO_SetLevel(PIN_FORCE_1V8_EN,false);
}

uint8_t GetButtonState(){
    return (GPIO_GetLevel(PIN_BUTTON1) << 7) |
        (GPIO_GetLevel(PIN_BUTTON2) << 6) |
        (GPIO_GetLevel(PIN_BUTTON3) << 5) |
        (GPIO_GetLevel(PIN_BUTTON4) << 4) |
        (GPIO_GetLevel(PIN_BUTTON5) << 3) |
        (GPIO_GetLevel(PIN_BUTTON6) << 2) |
        (GPIO_GetLevel(PIN_BUTTON7) << 1) |
        GPIO_GetLevel(PIN_BUTTON_POWER);
}

float ParseJoystick(float input){
    float vsMid = input - JOYSTICK_MID;
  
    if(vsMid > -JOYSTICK_DEADBAND && vsMid < JOYSTICK_DEADBAND) return 0; //Deadband

    float toReturn = (((input - JOYSTICK_MINVAL)/(JOYSTICK_MAXVAL-JOYSTICK_MINVAL)) * 2) -1;
    if(toReturn < -1) toReturn = -1;
    if(toReturn > 1) toReturn = 1;

    return toReturn;
}

void GetJoysticks(float* joystickX, float* joystickY){
    GPIO_SetLevel(PIN_JOY_ON,true);
   
   // DelayMS(10); //Just in case

    ADCsampleDone[0]=false;
    ADCsampleDone[1]=false;

    am_hal_adc_sw_trigger(g_ADCHandle);
    
    while(!ADCsampleDone[0]) {};
    *joystickX = ParseJoystick(lastADCsample[1]);
    //LOG_W("%.3f %.3f",lastADCsample[1],lastADCsample[0]);
    while(!ADCsampleDone[1]) {};
    *joystickY = -ParseJoystick(lastADCsample[0]);

    //GPIO_SetLevel(PIN_JOY_ON,false);    
}

float k_GetVBat(){
    ADCsampleDone[2]=false;
    am_hal_adc_sw_trigger(g_ADCHandle);
    while(!ADCsampleDone[2]) {};
    return lastADCsample[2] / 0.18032f;
}

void TurnOffIfNeccessary(){
    if(GPIO_GetLevel(PIN_BUTTON_POWER)==1){
        LOG_I("Shutting down...");
        DelayMS(1000);
        GPIO_SetLevel(PIN_FORCE_1V8_EN,false); //Stop requestion 1V8 power 
        while(true){} //Spin loop forever :( (The power should die soon)
    }
}


void InitADC(){
    _Static_assert(PIN_JOY_1==19 && PIN_JOY_2==18 && PIN_VBAT==12);  // .GP.cfg_b.uFuncSel might change on different pins

    const am_hal_gpio_pincfg_t g_ADC_PIN_CFG = { .GP.cfg_b.uFuncSel = AM_HAL_PIN_19_ADCSE0 };
    am_hal_gpio_pinconfig(PIN_JOY_1, g_ADC_PIN_CFG);

    const am_hal_gpio_pincfg_t g_ADC_PIN_CFG_2 = { .GP.cfg_b.uFuncSel = AM_HAL_PIN_18_ADCSE1 };
    am_hal_gpio_pinconfig(PIN_JOY_2, g_ADC_PIN_CFG_2);

    const am_hal_gpio_pincfg_t g_ADC_PIN_CFG_3 = { .GP.cfg_b.uFuncSel = AM_HAL_PIN_12_ADCSE7 };
    am_hal_gpio_pinconfig(PIN_VBAT, g_ADC_PIN_CFG_3);

    am_hal_adc_config_t           ADCConfig;
    am_hal_adc_slot_config_t      ADCSlotConfig;

    am_hal_adc_initialize(0, &g_ADCHandle);    
    //
    // Get the ADC correction offset and gain for this DUT.
    // Note that g_fTrims[3] must contain the value -123.456f before calling
    // the function.
    // On return g_fTrims[0] contains the offset, g_fTrims[1] the gain.
    //
    g_fTrims[0] = g_fTrims[1] = g_fTrims[2] = 0.0F;
    g_fTrims[3] = -123.456f;
    am_hal_adc_control(g_ADCHandle, AM_HAL_ADC_REQ_CORRECTION_TRIMS_GET, g_fTrims);

    am_hal_adc_power_control(g_ADCHandle, AM_HAL_SYSCTRL_WAKE,false);

    //
    // Set up the ADC configuration parameters. These settings are reasonable
    // for accurate measurements at a low sample rate.
    //
    ADCConfig.eClock             = AM_HAL_ADC_CLKSEL_HFRC_24MHZ;
    ADCConfig.ePolarity          = AM_HAL_ADC_TRIGPOL_RISING;
    ADCConfig.eTrigger           = AM_HAL_ADC_TRIGSEL_SOFTWARE;
    ADCConfig.eClockMode         = AM_HAL_ADC_CLKMODE_LOW_LATENCY;
    ADCConfig.ePowerMode         = AM_HAL_ADC_LPMODE0;
    ADCConfig.eRepeat            = AM_HAL_ADC_SINGLE_SCAN;
    ADCConfig.eRepeatTrigger     = AM_HAL_ADC_RPTTRIGSEL_INT;
    am_hal_adc_configure(g_ADCHandle, &ADCConfig);

    //
    // Set up an ADC slot
    //

    //! Set additional input sampling ADC clock cycles
    ADCSlotConfig.eMeasToAvg      = AM_HAL_ADC_SLOT_AVG_1;
    ADCSlotConfig.ui32TrkCyc      = AM_HAL_ADC_MIN_TRKCYC;
    ADCSlotConfig.ePrecisionMode  = AM_HAL_ADC_SLOT_12BIT;
    ADCSlotConfig.eChannel        = AM_HAL_ADC_SLOT_CHSEL_SE1;
    ADCSlotConfig.bWindowCompare  = false;
    ADCSlotConfig.bEnabled        = true;

    ADCSlotConfig.eChannel = AM_HAL_ADC_SLOT_CHSEL_SE0;
    am_hal_adc_configure_slot(g_ADCHandle, 0, &ADCSlotConfig);

    ADCSlotConfig.eChannel = AM_HAL_ADC_SLOT_CHSEL_SE1;
    am_hal_adc_configure_slot(g_ADCHandle, 1, &ADCSlotConfig);

    ADCSlotConfig.eChannel = AM_HAL_ADC_SLOT_CHSEL_SE7;
    am_hal_adc_configure_slot(g_ADCHandle, 2, &ADCSlotConfig);


    //
    // For this example, the samples will be coming in slowly. This means we
    // can afford to wake up for every conversion.
    //
    am_hal_adc_interrupt_enable(g_ADCHandle, AM_HAL_ADC_INT_FIFOOVR1 | AM_HAL_ADC_INT_DERR | AM_HAL_ADC_INT_DCMP | AM_HAL_ADC_INT_CNVCMP | AM_HAL_ADC_INT_SCNCMP );

    //
    // Enable the ADC.
    //
    am_hal_adc_enable(g_ADCHandle);

    //
    // Enable internal repeat trigger timer
    //
    am_hal_adc_irtt_enable(g_ADCHandle);

    NVIC_SetPriority(ADC_IRQn, AM_IRQ_PRIORITY_DEFAULT);
    NVIC_EnableIRQ(ADC_IRQn);
    am_hal_interrupt_master_enable();
}

void am_adc_isr(void)
{
    uint32_t ui32IntStatus;
    
    // Clear the ADC interrupt.
    am_hal_adc_interrupt_status(g_ADCHandle, &ui32IntStatus, true);
    am_hal_adc_interrupt_clear(g_ADCHandle, ui32IntStatus);   

    // Get samples until the FIFO is emptied.    

    while(AM_HAL_ADC_FIFO_COUNT(ADC->FIFO))
    {        
        am_hal_adc_sample_t sample;
        uint32_t sampleCount=1;
        // Invalidate DAXI to make sure CPU sees the new data when loaded.        
        am_hal_daxi_control(AM_HAL_DAXI_CONTROL_INVALIDATE, NULL);
        am_hal_adc_samples_read(g_ADCHandle, false, NULL, &sampleCount, &sample);

        if(sample.ui32Slot<=2){
            lastADCsample[sample.ui32Slot] = (float)(sample.ui32Sample  * AM_HAL_ADC_VREFMV / 0x1000) /1000;
            ADCsampleDone[sample.ui32Slot] = true;
        }else{
            LOG_W("Unsupported ADC slot!");
        }  
    }
}

//From https://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to-reverse-the-order-of-bits-in-a-bytes
uint8_t reverseByte(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void SetupBacklightPWM(int pin, uint8_t initialValue){

    am_hal_gpio_pincfg_t pinConfig = {0};     
    pinConfig.GP.cfg_b.uFuncSel = AM_HAL_PIN_33_CT33; // Set GPIO function
    pinConfig.GP.cfg_b.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
    pinConfig.GP.cfg_b.eGPOutCfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
    pinConfig.GP.cfg_b.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA;
    am_hal_gpio_pinconfig(pin, pinConfig);


    am_hal_timer_config_t timerConfig = {
        .eFunction = AM_HAL_TIMER_FN_PWM,
        .bInvertOutput0 = false,
        .bInvertOutput1 = false,
        .bLowJitter = false,
        .eInputClock = AM_HAL_TIMER_CLOCK_HFRC_DIV1024, //1024 = 367Hz
        .eTriggerType = AM_HAL_TIMER_TRIGGER_DIS,
        .ui32Compare0 = 255,
        .ui32Compare1 = 0,
        .ui32PatternLimit = 0
    };
    am_hal_timer_config(0,&timerConfig);
   
    am_hal_timer_output_config(pin, AM_HAL_TIMER_OUTPUT_TMR0_OUT0);

    ChangeBacklightPWMValue(initialValue);

    am_hal_timer_enable(0);
    am_hal_timer_start(0);
}

void ChangeBacklightPWMValue(uint8_t value){
    am_hal_timer_compare1_set(0, 255 - value);    
}