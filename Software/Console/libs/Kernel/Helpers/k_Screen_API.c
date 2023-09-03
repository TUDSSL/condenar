#include "Screen/Screen.h"
#include "Kernel.h"
#include "Fonts/font8x8.h"
#include "stdarg.h"
#include "Screen/ScreenConfig.h"
#include "Misc.h"
#include "PinDefinitions.h"
#include "Helpers/Metrics.h"
#include "string.h"
#include "Helpers/Flash.h"

uint8_t Crank_Icon[];
uint8_t BatteryFull_Icon[];


//APIs that should be hardware agnostic (ONLY using the DRAW_SetPixel method to write to the frame)
//These are OK to be slow, mainly only used for menus and things like that

inline void k_draw_SetPixel(int32_t x, int32_t y, k_color color){
    if(x < 0 || y < 0 || x >= SCREEN_WIDTH_REAL || y >= SCREEN_HEIGHT_REAL) return;

#ifdef K_SCREEN_RGB
    #error "RGB Not implemented"
#else
    DRAW_SetPixel(x, y, color);
#endif    
}


void displayPutChar(char c, int x, int y)
{
    for (int cy = 0; cy < FONT_HEIGHT; cy++)
    {
        uint8_t fb = font8x8_basic[0x7F & c][cy];
        for (int cx = 0; cx < FONT_WIDTH; cx++)
        {   
            int xPos = x+cx;
            int yPos = y+cy;
            if(xPos<0 || xPos>=SCREEN_WIDTH_REAL || yPos<0 || yPos>=SCREEN_HEIGHT_REAL) continue;

            DRAW_SetPixel(x + cx, y + cy, (fb & 1) ? K_COLOR_BLACK : K_COLOR_WHITE);
            fb >>= 1;
        }
    }
}

//PRINTF APIs
void displayVPrintf(int x, int y, const char *format, va_list va, bool centered)
{
    int MAX_STRING_SIZE = 128;
    char outString[MAX_STRING_SIZE];
    int printed = am_util_stdio_vsnprintf(outString, MAX_STRING_SIZE, format, va);
    outString[printed] = '\0';
    if(centered) x-=(FONT_WIDTH*printed)/2;
    for (int i = 0;
            i < MAX_STRING_SIZE && x < SCREEN_WIDTH_REAL && outString[i] <= 0x7F && outString[i] > 0;
            i++)
    {
        displayPutChar(outString[i], x, y);
        x += FONT_WIDTH;
    }
}
void displayPrintf(int x, int y, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    displayVPrintf(x, y, format, va, false);
    va_end(va);
}
//END OF PRINTF APIs

void k_draw_Stats();
void k_DrawOverlays(){
    k_draw_Stats();
    k_draw_Metrics();
}


void k_draw_Clear(k_color color){
    #ifdef FAST_CLEAR_IMPLEMENTED
    DRAW_FastClearScreen(color);
    #else
    for(int i=0;i<SCREEN_WIDTH_REAL;i++){
        for(int k=0;k<SCREEN_HEIGHT_REAL;k++){
            k_draw_SetPixel(i,k,color);
        }
    }
    #endif    
}

void k_draw_HLine(int32_t x1, int32_t x2, int32_t y, k_color color){
    for(int i=x1;i<=x2;i++){
        k_draw_SetPixel(i, y, color);
    }
}

void k_draw_VLine(int32_t x, int32_t y1, int32_t y2, k_color color){
    for(int i=y1;i<=y2;i++){
        k_draw_SetPixel(x, i, color);
    }
}

void k_draw_DrawRectangle(int32_t x, int32_t y, int32_t width, int32_t height, k_color color){
    k_draw_HLine(x,x+width-1,y,color);
    k_draw_HLine(x,x+width-1,y+height-1,color);
    k_draw_VLine(x,y,y+height-1,color);
    k_draw_VLine(x+width-1,y,y+height-1,color);
}

void k_draw_FillRectangle(int32_t x, int32_t y, int32_t width, int32_t height, k_color color){
    for(int cy=0;cy<height;cy++){
        k_draw_HLine(x,x+width-1,y+cy,color);
    }
}


void k_draw_VDrawTextLeftAligned(int32_t x,int32_t y, int sizeMultiplier, const char *format, va_list args){
    displayVPrintf(x, y, format, args, false);
}

void k_draw_DrawTextLeftAligned(int32_t x,int32_t y, int sizeMultiplier, const char *format, ...){
    va_list va;
    va_start(va, format);
    k_draw_VDrawTextLeftAligned(x, y, sizeMultiplier, format, va);
    va_end(va);
}

void k_draw_VDrawTextCentered(int32_t centerX,int32_t y, int sizeMultiplier, const char *format, va_list args) {
     displayVPrintf(centerX, y, format, args,true);
}
void k_draw_DrawTextCentered(int32_t centerX,int32_t y, int sizeMultiplier, const char *format, ...){
    va_list va;
    va_start(va, format);
    k_draw_VDrawTextCentered(centerX,y,sizeMultiplier,format,va);   
    va_end(va);
}

void k_draw_DrawImage(int32_t x,int32_t y,int32_t imageWidth,int32_t imageHeight, uint8_t scale, uint8_t* image){
#ifdef FAST_DRAW_IMAGE_BW_IMPLEMENTED
    DRAW_FastImage_BW(x,y,imageWidth,imageHeight,scale, image);
#else
    int bytesPerLine = (imageWidth / 8) + ((imageWidth & 0x07) != 0); //Fast int ceiling
    for(int cy=0;cy<imageHeight;cy++){
        for(int cx=0;cx<imageWidth;cx++){
            int byteNum = cy*bytesPerLine + (cx/8);
            int byteOffset= cx % 8;
            bool color = (image[byteNum] >> (7-byteOffset)) & 0x01;

            for(int sx=0;sx<scale;sx++){
                for(int sy=0;sy<scale;sy++){
                    k_draw_SetPixel(x+(cx*scale) +sx, y+(cy*scale) +sy, color ? K_COLOR_WHITE : K_COLOR_BLACK);
                }
            }
            
        }
    }
#endif
}

void k_draw_DrawImageRGB_888(int32_t x,int32_t y,int32_t imageWidth,int32_t imageHeight, uint8_t scale, unsigned long* image){
    for(int cy=0;cy<imageHeight;cy++){
        for(int cx=0;cx<imageWidth;cx++){
            int byteNum = cy*imageWidth + cx;
            unsigned long colorItem = image[byteNum];
            k_color color;
            color.r = colorItem >> 16;
            color.g = colorItem >> 8;
            color.b = colorItem;

            for(int sx=0;sx<scale;sx++){
                for(int sy=0;sy<scale;sy++){
                    k_draw_SetPixel(x+(cx*scale) +sx, y+(cy*scale) +sy, color);
                }
            }
            
        }
    }
}


void Screen_IMPL_SetLine(int y, k_color* data, int lineOffset, int pixelToSet);
void k_draw_SetPixel(int32_t x, int32_t y, k_color color);
void k_draw_DrawImageRGBA_2221(int32_t x,int32_t y,int32_t imageWidth,int32_t imageHeight, uint8_t scale, uint8_t* image){
#ifdef FAST_DRAW_IMAGE_RGB2221_IMPLEMENTED
    DRAW_FastImage_2221(x,y,imageWidth,imageHeight,scale, image);
#else
    for(int cy=0;cy<imageHeight;cy++){
        for(int cx=0;cx<imageWidth;cx++){
            int byteNum = cy*imageWidth + cx;
            uint8_t colorItem = GET_XIP_CHACHED_VALUE_8(image+byteNum);
            k_color color;
            color.r = (colorItem & 0b11000000);
            color.g = (colorItem & 0b00110000) << 2;
            color.b = (colorItem & 0b00001100) << 4;
            bool alpha = (colorItem & 0b00000011) > 0;

            color.r = color.r | (color.r >> 2) | (color.r >> 4)| (color.r >> 6); //Expand 2 bits to 8 bits
            color.g = color.g | (color.g >> 2) | (color.g >> 4)| (color.g >> 6);
            color.b = color.b | (color.b >> 2) | (color.b >> 4)| (color.b >> 6);

            for(int sx=0;sx<scale;sx++){
                for(int sy=0;sy<scale;sy++){
                    if(alpha) k_draw_SetPixel(x+(cx*scale) +sx, y+(cy*scale) +sy, color);
                }
            }            
        }
    }
#endif   
}

void k_draw_DrawImageGeneric(k_image* image, int32_t x,int32_t y, uint8_t scale){
    if(image->ready==false) return;

    if(image==NULL) return;

    if(x>SCREEN_WIDTH_REAL || y>SCREEN_HEIGHT_REAL) return;
    if(x+image->width*scale<0 || y+image->height*scale<0) return;

    switch (image->format)
    {
    case K_IMAGE_FORMAT_RGBA2221:
        k_draw_DrawImageRGBA_2221(x,y, image->width, image->height, scale, image->dataPtr);
        break;    
    default:
        LOG_W("Unknown image format: %d", image->format);
        break;
    }
}

void k_draw_DrawLoadingScreen(k_app* app, uint8_t progressPercent, const char *format, va_list args){
    k_draw_Clear(K_COLOR_WHITE);
    //App icons are 50x50, so lets render them in 3x (150x150)
    if(app->appIconImage.ready) k_draw_DrawImageGeneric(&app->appIconImage, (SCREEN_WIDTH_REAL/2) - 96, -10,3);

    int barWidth= (SCREEN_WIDTH_REAL / 3)*2;
    int barHeight = 17;
    k_draw_DrawRectangle((SCREEN_WIDTH_REAL/2) - (barWidth/2) ,20+150+10,barWidth,barHeight,K_COLOR_BLACK);

    int internalBarWidth=barWidth-4;
    int internalBarWidthAtThisProgress = (int)(internalBarWidth * (progressPercent/100.0f));
    k_draw_FillRectangle((SCREEN_WIDTH_REAL/2) - (barWidth/2) +2 ,20+150+10 + 2,internalBarWidthAtThisProgress,barHeight-4,K_COLOR_BLACK);
    k_draw_VDrawTextCentered(SCREEN_WIDTH_REAL/2,20+150+10+barHeight+6,1,format, args);
}

void k_screen_RenderLoadingScreen(uint8_t progressPercent,const char *format, ...){
    k_app* currentApp = k_getCurrentApp();
    if(currentApp==NULL){
        LOG_W("k_getCurrentApp returned null when rendering loading screen!");
        return;
    }

    k_BeginScreenUpdate();
    va_list va;
    va_start(va, format);
    k_draw_DrawLoadingScreen(currentApp, progressPercent, format, va);
    va_end(va);
    
    k_EndScreenUpdate(false,true); //No checkpoint on loading screens
}

int counter=0;
void k_draw_Stats(){

    uint32_t fps = k_getLastFPS();

    if(k_GetSettingBool("/Other/Debugging/Overlay/Show FPS", false)){
        k_draw_FillRectangle(0,0,50,11,K_COLOR_WHITE);
        k_draw_DrawTextLeftAligned(2,2,1,"FPS:%02d",fps);
    }

    if(k_GetSettingBool("/Other/Debugging/Overlay/Show battery %%", true)){

        float percent = (((k_GetVBat()*k_GetVBat())/(4.7*4.7))*107) - 7;
        if(percent<0) percent=0;
        if(percent>99) percent=99;

        /*k_draw_FillRectangle(269,0,51,13,K_COLOR_WHITE);        
        k_draw_DrawImage(270,1,20,11,1,BatteryFull_Icon);
        k_draw_DrawTextLeftAligned(269 + 2+20+3,3,1,"%d%%",(int)percent);*/

        k_draw_FillRectangle(269 + 2+20+3-8*4-2,0,71,11,K_COLOR_WHITE);        
        k_draw_DrawTextLeftAligned(269 + 2+20+4-8*4,2,1,"-|%d%%|-",(int)percent);
    }

     if(k_GetSettingBool("/Other/Debugging/Overlay/Show crank status", true) && k_IsBeingCranked()){  
        k_draw_DrawImage(298,13,20,20,1,Crank_Icon);
    }

    if(k_GetSettingBool("/Other/Debugging/Overlay/Show heap modifications", false)){
        int numRegions = 64;
        int regionsRectHeight = 10;
        int rectWidth=3;
        int rectSeparation = 1;
        int totalWidth = rectWidth * numRegions + rectSeparation * (numRegions-1) + 2 + 2;
        int regionsRectStartX = (SCREEN_WIDTH_REAL/2) - (totalWidth/2);
        int regionsRectStartY = (SCREEN_HEIGHT_REAL) - regionsRectHeight - 1;

        k_memRegions regions;
        k_MemWatcher_GetModifiedRegions(&regions);

        k_draw_FillRectangle(regionsRectStartX-1,regionsRectStartY-1,totalWidth+2,regionsRectHeight+2,K_COLOR_WHITE);
        k_draw_DrawRectangle(regionsRectStartX,regionsRectStartY,totalWidth,regionsRectHeight,K_COLOR_BLACK);
        for(int i=0;i<numRegions;i++){
            int regionVal=regions.regionModifiedStatus[i/8];
            bool regionModified = (regionVal & (1<<(7-(i%8))))>0;
            k_draw_FillRectangle(regionsRectStartX + 2 + (i*(rectWidth+rectSeparation)),regionsRectStartY + 2,rectWidth, regionsRectHeight - 4,regionModified ? K_COLOR_BLACK : K_COLOR_WHITE);
        }
    }
}


//Bresenham circle part function. Adapted from: https://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/
void drawBresenhamCircle(int xc, int yc, int x, int y, k_color color)
{
	k_draw_SetPixel(xc+x, yc+y, color);
	k_draw_SetPixel(xc-x, yc+y, color);
	k_draw_SetPixel(xc+x, yc-y, color);
	k_draw_SetPixel(xc-x, yc-y, color);
	k_draw_SetPixel(xc+y, yc+x, color);
	k_draw_SetPixel(xc-y, yc+x, color);
	k_draw_SetPixel(xc+y, yc-x, color);
	k_draw_SetPixel(xc-y, yc-x, color);
}


//Bresenham’s Algorithm for drawing a circle. Adapted from: https://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/
void k_draw_DrawCircle(int32_t xCenter, int32_t yCenter, int32_t radius, k_color color){
    int x = 0, y = radius;
	int d = 3 - 2 * radius;
	drawBresenhamCircle(xCenter, yCenter, x, y, color);
	while (y >= x)
	{		
		x++;
		if (d > 0)
		{
			y--;
			d = d + 4 * (x - y) + 10;
		}
		else
			d = d + 4 * x + 6;
		drawBresenhamCircle(xCenter, yCenter, x, y, color);
	}
}


void k_draw_FillCircle(int32_t xCenter, int32_t yCenter, int32_t radius, k_color color){
    //Just bruteforce it (Check all points to see if they are inside the radius)
    for(int y=-radius; y<=radius; y++)
    {
        for(int x=-radius; x<=radius; x++)
        {
            if(x*x+y*y <= radius*radius + radius) k_draw_SetPixel(xCenter+x, yCenter+y,color);
        }
    }       
}



CHECKPOINT_EXCLUDE_DATA uint8_t Crank_Icon[] = { //20x20
  	0xff, 0xff, 0xf0, 0xfc, 0xf3, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 0xf0, 
	0x00, 0xf0, 0xe0, 0x00, 0x70, 0x80, 0xf0, 0x10, 0x81, 0xf8, 0x10, 0x81, 0xf8, 0x10, 0x81, 0xf8, 
	0x10, 0x81, 0xf8, 0x10, 0xc0, 0xf0, 0x30, 0xf0, 0x00, 0x70, 0xf0, 0x00, 0xf0, 0xf0, 0x00, 0xf0, 
	0xf0, 0x00, 0xf0, 0xf0, 0x60, 0xf0, 0xfc, 0xf3, 0xf0, 0xff, 0xff, 0xf0
};

CHECKPOINT_EXCLUDE_DATA uint8_t BatteryFull_Icon[] = { //20x11
	0x00, 0x00, 0x70, 0x7f, 0xff, 0x70, 0x64, 0x93, 0x70, 0x64, 0x93, 0x00, 0x64, 0x93, 0x40, 0x64, 
	0x93, 0x40, 0x64, 0x93, 0x40, 0x64, 0x93, 0x00, 0x64, 0x93, 0x70, 0x7f, 0xff, 0x70, 0x00, 0x00, 
	0x70
};