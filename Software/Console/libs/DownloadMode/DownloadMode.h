#ifndef DOWNLOADMODE_H
#define DOWNLOADMODE_H

#include "Kernel.h"
#include <stdint.h>
#include <stdbool.h>

void DownloadModeEntryPoint(); 

void DownloadMode_DoWork();

void uart_print(char *pcStr); //Defined in main.c (Used for printf)

uint32_t CalculateChecksum(void* buffer,int length);

extern uint8_t DownloadMode_Icon[];

#endif