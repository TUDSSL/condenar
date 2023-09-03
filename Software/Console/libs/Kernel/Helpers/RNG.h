#ifndef RNG_H
#define RNG_H

#include <stdint.h>

void InitRNG();
uint8_t GetRandomByte();
uint32_t GetRandomInt();

#endif