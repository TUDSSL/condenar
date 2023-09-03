#include "stdint.h"

//This doesn't return if successful (so it should be called after all peripherals have been initialized)
void k_tryRecoverFromPowerDown();

void k_storeCurrentState();


