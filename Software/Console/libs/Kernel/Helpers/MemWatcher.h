#ifndef MEMWATCHER_H
#define MEMWATCHER_H

#include <stdint.h>
#include <stdbool.h>

void MemWatcher_Initialize();
void MemWatcher_SetEnabled(bool enabled);

#endif