#ifndef KERNEL_DRAW_H
#define KERNEL_DRAW_H

#include "Screen_Base.h"

void k_SetRawBufferByte(uint32_t index, uint8_t value);

//Renders and shows a loading screen for the current app
void k_screen_RenderLoadingScreen(uint8_t progressPercent,const char *format, ...);


void k_draw_Clear(k_color color);
void k_draw_SetPixel(int32_t x, int32_t y, k_color color);
void k_draw_VLine(int32_t x, int32_t y1, int32_t y2, k_color color);
void k_draw_HLine(int32_t x1, int32_t x2, int32_t y, k_color color);
void k_draw_DrawRectangle(int32_t x, int32_t y, int32_t width, int32_t height, k_color color);
void k_draw_DrawTextLeftAligned(int32_t x,int32_t y, int sizeMultiplier, const char *format, ...);
void k_draw_DrawTextCentered(int32_t centerX,int32_t centerY, int sizeMultiplier, const char *format, ...);
void k_draw_DrawImage(int32_t x,int32_t y,int32_t imageWidth,int32_t imageHeight, uint8_t scale, uint8_t* image);
void k_draw_DrawImageRGB_888(int32_t x,int32_t y,int32_t imageWidth,int32_t imageHeight, uint8_t scale, unsigned long* image);
void k_draw_DrawImageGeneric(k_image* image, int32_t x,int32_t y, uint8_t scale);
void k_draw_FillRectangle(int32_t x, int32_t y, int32_t width, int32_t height, k_color color);
void k_draw_DrawCircle(int32_t xCenter, int32_t yCenter, int32_t radius, k_color color);
void k_draw_FillCircle(int32_t xCenter, int32_t yCenter, int32_t radius, k_color color);

#endif