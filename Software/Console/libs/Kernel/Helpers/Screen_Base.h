#ifndef SCREEN_BASE_H
#define SCREEN_BASE_H

#include <stdint.h>

//Basic things both public and private

#define SCREEN_WIDTH_REAL 320
#define SCREEN_HEIGHT_REAL 240


typedef struct __attribute__((packed)) k_color{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} k_color;
#define K_COLOR_RGB(rValue, gValue, bValue) {.r=rValue, .g=gValue, .b=bValue}


static k_color K_COLOR_BLACK = K_COLOR_RGB(0,0,0);
static k_color K_COLOR_WHITE = K_COLOR_RGB(255,255,255);
static k_color K_COLOR_RED = K_COLOR_RGB(255,0,0);
static k_color K_COLOR_GREEN = K_COLOR_RGB(0,255,0);
static k_color K_COLOR_BLUE = K_COLOR_RGB(0,0,255);


typedef enum k_image_format{
    K_IMAGE_FORMAT_RGBA2221 = 0,
} k_image_format;

typedef struct k_image{
    bool ready;
    uint8_t* dataPtr;
    k_image_format format;
    uint16_t width;
    uint16_t height;
}k_image;

#endif