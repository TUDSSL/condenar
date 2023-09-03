#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "Kernel.h"

typedef enum FileSystemTypes{
    GB = 1,
    GBC = 2,
    GBA_BIOS = 4,
    RGBA2221_IMAGE = 8,
    GBA = 16,
} FileSystemTypes;

//Returns whether the file exists
bool GetFileByName(const char* name, uint8_t** addressOut, uint32_t* sizeOut, uint16_t* param1Out, uint16_t* param2Out, FileSystemTypes* fileTypeOut);

//Returns false if the user wants to exit. Otherwise, returns true and sets the addressOut and sizeOut to the file selected
bool ShowFileSelection(uint8_t fileType, uint8_t** addressOut, uint32_t* sizeOut, uint16_t* param1Out, uint16_t* param2Out);

k_image GetImageByName(const char* name);

#endif