#include "RNG.h"

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include "Misc.h"

uint32_t lfsrState = 0;
bool rngInitialized=false;

void InitRNG(){
    LOG_I("Initializing RNG...");
    /* init Rnd context's inner member */  
    am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_CRYPTO);
   
    CRYPTO->RNGSWRESET=1;
    CRYPTO->RNGCLKENABLE_b.EN=1;
    CRYPTO->TRNGCONFIG_b.SOPSEL=1;
    CRYPTO->RNDSOURCEENABLE_b.RNDSRCEN=1;

    

    //lfsrState=69;

}

uint8_t GetRandomByte(){
    return GetRandomInt();
}

//Xorshift components from https://en.wikipedia.org/wiki/Xorshift
uint32_t GetRandomInt(){
    
    if(!rngInitialized){
        while(!CRYPTO->TRNGVALID_b.EHRVALID){ }
        lfsrState = CRYPTO->EHRDATA0;
        rngInitialized=true;
        LOG_I("RNG initialized with seed %d...", lfsrState);
    }

    lfsrState ^= lfsrState << 13;
    lfsrState ^= lfsrState >> 17;
    lfsrState ^= lfsrState << 5;
    return lfsrState;
}
