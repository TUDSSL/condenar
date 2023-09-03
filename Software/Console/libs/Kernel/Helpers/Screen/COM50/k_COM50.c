#include "Screen/ScreenConfig.h"
#ifdef USE_COM50

#include "ScreenCOM50.h"
#include "Screen_Base.h"
#include "Flash.h"
#include "Misc.h" 
#include "string.h"

void Screen_IMPL_SendFrame();

//#define DIRECT_DRAWING
#define SCREEN_BUFFER_SIZE (SCREEN_WIDTH_REAL*SCREEN_HEIGHT_REAL*sizeof(k_color))

k_color* screenBuffers[2] = {(k_color*)EXT_RAM_BASE_ADDRESS, (k_color*)(EXT_RAM_BASE_ADDRESS + SCREEN_BUFFER_SIZE)};
k_color* activeScreenBuffer; //Buffer where writes are made
k_color* standbyScreenBuffer; //Buffer that is being displayed

//k_color* screenBuffer = (k_color*)(SRAM_XIP_BASE_ADDRESS + (1024*1024) - 250000 + 1024);

void Screen_IMPL_Initialize(){   

    activeScreenBuffer = screenBuffers[0];
    standbyScreenBuffer = screenBuffers[1];

    memset(activeScreenBuffer,0,SCREEN_WIDTH_REAL*SCREEN_WIDTH_REAL*sizeof(k_color));
    memset(standbyScreenBuffer,0,SCREEN_WIDTH_REAL*SCREEN_WIDTH_REAL*sizeof(k_color));

    SSD1963_Initialize();
    COM50_Initialize();

    //Screen_IMPL_SendFrame();
}


void Screen_IMPL_WaitForPendingFrames() //Waits for either a VSYNC interval or (if internally buffered) until the last frame was sent
{
#ifndef DIRECT_DRAWING
    SSD1963_WaitForAsync();  
#endif  
}



void Screen_IMPL_SendLine(int y, k_color* data, int pixelToSet){
    SSD1963_SetupDrawRegion(0,320,y,y);
    SSD1963_SendToDrawRegion(true, data, pixelToSet);
}


void Screen_IMPL_SendFrame() //Forces the buffered frame to be sent (probably in an async way), or does nothing if not using a buffered approach
{ 
#ifndef DIRECT_DRAWING
    /*for(int i=0;i<SCREEN_HEIGHT_REAL;i++){
        Screen_IMPL_SendLine(i, screenBuffer + (320*i));      
    }*/
    /*SSD1963_SetupDrawRegion(0,SCREEN_WIDTH_REAL,0,SCREEN_HEIGHT_REAL);
    SSD1963_SendToDrawRegion(true, screenBuffer,SCREEN_WIDTH_REAL*SCREEN_HEIGHT_REAL);*/

    Screen_IMPL_WaitForPendingFrames();

    //Now that we are done sending the frame, swap the buffers
    k_color* temp = activeScreenBuffer;
    activeScreenBuffer = standbyScreenBuffer;
    standbyScreenBuffer = temp;

    //Send the stanby buffer to the screen (the one that was just being written to)
    SSD1963_SetupDrawRegion(0,SCREEN_WIDTH_REAL-1,0,SCREEN_HEIGHT_REAL);
    SSD1963_SendToDrawRegionAsync(true, standbyScreenBuffer,SCREEN_WIDTH_REAL*SCREEN_HEIGHT_REAL);
    
#endif    
}

void DRAW_SetPixel(int x, int y, k_color color){
#ifdef DIRECT_DRAWING
    SSD1963_SetupDrawRegion(x,x,y,y);
    SSD1963_SendToDrawRegion(true, &color, 1 );
#else 
    activeScreenBuffer[(y*SCREEN_WIDTH_REAL) + x] = color; //color;
#endif
}

void Screen_IMPL_SetLine(int y, k_color* data, int lineOffset, int pixelToSet){
#ifdef DIRECT_DRAWING
    Screen_IMPL_SendLine(y, &data, pixelToSet);
#else 
    memcpy(activeScreenBuffer+(y*SCREEN_WIDTH_REAL) + lineOffset,data, pixelToSet * sizeof(k_color));
#endif
}

void DRAW_FastClearScreen(k_color color){
    #ifdef DIRECT_DRAWING
    SSD1963_SetupDrawRegion(0,SCREEN_WIDTH_REAL,0,SCREEN_HEIGHT_REAL);
    SSD1963_SetToDrawRegion(true, color,SCREEN_WIDTH_REAL * SCREEN_HEIGHT_REAL);
    #else 
    if(memcmp(&color, &K_COLOR_WHITE, sizeof(k_color))==0){
        memset(activeScreenBuffer,0xFF,sizeof(k_color)*SCREEN_WIDTH_REAL * SCREEN_HEIGHT_REAL);
    }else if(memcmp(&color, &K_COLOR_BLACK, sizeof(k_color))==0){
        memset(activeScreenBuffer,0x00,sizeof(k_color)*SCREEN_WIDTH_REAL * SCREEN_HEIGHT_REAL);
    }else{
        for(int y=0;y<SCREEN_HEIGHT_REAL;y++){
            for(int x = 0;x<SCREEN_WIDTH_REAL;x++){
                activeScreenBuffer[(y*SCREEN_WIDTH_REAL) + x] = color; //color;
            }
        }
    }
  
    #endif
}

void DRAW_FastImage_BW(int32_t x,int32_t y,int32_t imageWidth,int32_t imageHeight, uint8_t scale, uint8_t* image){
     
    k_color lineBuffer[320]; //This needs to be at least the screen WIDTH
    int bufferAmountUsed = 0;
    
    int totalWidth = imageWidth * scale;
    int totalHeight = imageHeight * scale;

    int bytesPerLine = (imageWidth / 8) + ((imageWidth & 0x07) != 0); //Fast int ceiling

    int cY = MAX(0,y);
    while(cY < MIN(y+totalHeight, SCREEN_HEIGHT_REAL)){
        int imagePixelY = (cY - y)/scale;
        int cX = MAX(0,x);
        while(cX < MIN(x+totalWidth, SCREEN_WIDTH_REAL)){            
            int imagePixelX = (cX - x)/scale;

            int byteNum = imagePixelY*bytesPerLine + (imagePixelX/8);
            int byteOffset= imagePixelX % 8;
            bool color = (image[byteNum] >> (7-byteOffset)) & 0x01;
            
            lineBuffer[bufferAmountUsed] = color ? K_COLOR_WHITE : K_COLOR_BLACK;
            bufferAmountUsed++;

            cX++;
        }
#ifdef DIRECT_DRAWING
        SSD1963_SetupDrawRegion(MAX(0,x),MAX(0,x)+bufferAmountUsed,cY,cY+1);
        SSD1963_SendToDrawRegion(true, lineBuffer,totalWidth);              
#else       
        memcpy(&activeScreenBuffer[SCREEN_WIDTH_REAL * cY + MAX(0,x)], lineBuffer, (bufferAmountUsed)*sizeof(k_color));
#endif
        bufferAmountUsed = 0;   

        cY++;
    }  
}

#endif