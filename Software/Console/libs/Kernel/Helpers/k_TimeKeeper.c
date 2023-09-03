#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "Kernel.h"

#include "TimeKeeper.h"

#define WAKE_INTERVAL_IN_MS     1
#define XT_PERIOD               32768
#define WAKE_INTERVAL           (XT_PERIOD * WAKE_INTERVAL_IN_MS * 1e-3)

CHECKPOINT_INCLUDE_BSS volatile uint32_t currentTimeMS=0;

void am_stimer_cmpr0_isr(void)
{
    //
    // Check the timer interrupt status.
    //
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);
    am_hal_stimer_compare_delta_set(0, WAKE_INTERVAL);

    currentTimeMS+=WAKE_INTERVAL_IN_MS;
}

uint32_t millis(){
    return currentTimeMS;
}


uint32_t micros(){
    uint32_t currentCount = am_hal_stimer_counter_get();
    uint32_t currentCompare = am_hal_stimer_compare_get(0);
    int32_t compareLeft = currentCompare - currentCount;
    int32_t comparingDone = WAKE_INTERVAL - compareLeft;
    int32_t compareDoneUS = (comparingDone * 1000) / WAKE_INTERVAL;
    return currentTimeMS*1000 + compareDoneUS;
}


void Timer_Init(){
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);
    NVIC_EnableIRQ(STIMER_CMPR0_IRQn);
    am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
    am_hal_stimer_compare_delta_set(0, WAKE_INTERVAL);
    am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
    am_hal_interrupt_master_enable();
}