#include "Kernel.h"
#include "Helpers/PinDefinitions.h"
#include "Helpers/Misc.h"
#include "Helpers/TimeKeeper.h"
#include <math.h>

CHECKPOINT_INCLUDE_BSS uint32_t lastTimeAnyInputSeen = 0; //Intentionally not checkpointed

void k_GetInput(k_input_state* state, bool includeJoysticks){

    state->triggerRight = GPIO_GetLevel(PIN_BUTTON5);
    state->triggerLeft = GPIO_GetLevel(PIN_BUTTON6);

    state->buttonA = GPIO_GetLevel(PIN_BUTTON4);
    state->buttonB = GPIO_GetLevel(PIN_BUTTON1);
    state->buttonX = GPIO_GetLevel(PIN_BUTTON3);
    state->buttonY = GPIO_GetLevel(PIN_BUTTON2);

    //state->buttonJoystick = GPIO_GetLevel(PIN_BUTTON7);
    state->buttonPower = !GPIO_GetLevel(PIN_BUTTON_POWER);
  
    if(includeJoysticks) GetJoysticks(&state->joystickX,&state->joystickY);

    if(state->buttonA || state->buttonB || state->buttonX || state->buttonY
     || (includeJoysticks && (state->joystickX>0.01 || state->joystickY>0.01 || state->joystickX<-0.01 || state->joystickY<-0.01))
      || state->triggerLeft || state->triggerRight){
        lastTimeAnyInputSeen = millis();
    }
}

uint32_t k_getLastTimeInputSeen(){
    return lastTimeAnyInputSeen;
}