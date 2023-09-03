#include "stdint.h"

void k_copyToMRAM(uint8_t* dst, uint8_t* src, uint32_t size);
void k_setToMRAM(uint8_t* dst, uint8_t value, uint32_t size);