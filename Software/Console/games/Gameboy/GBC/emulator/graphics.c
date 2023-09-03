#include <assert.h>
#include "gb.h"
#include "Helpers/Flash.h"
#include "Kernel_Draw.h"

#define GBC_UPSCALE

#define GB_SKIP_FREQUENCY 3

CHECKPOINT_EXCLUDE_BSS int gbc_frameCount = 0;

uint8_t frameCountToSkip = 0;

inline bool IsGBCSkippingThisFrame(){
     return frameCountToSkip > 0;
}

k_color* GBC_screenBuffer =  (k_color*)(SRAM_XIP_BASE_ADDRESS + 256000); //[GB_LCD_WIDTH * GB_LCD_HEIGHT];
void Screen_IMPL_SetLine(int y, k_color* data, int lineOffset, int pixelToSet);

void gb_sdl_draw_line_dmg(struct gb *gb, unsigned ly, union gb_gpu_color line[GB_LCD_WIDTH]) {

     //Only render 1/2 of the frames since the GB wants to run at 60FPS
     if(IsGBCSkippingThisFrame()) return;

     unsigned i;
     unsigned x;
     unsigned y;

     static const k_color col_map[4] = {
          [GB_COL_WHITE]     = K_COLOR_RGB(0x75,0xA3,0x2C), //0xff75a32c,
          [GB_COL_LIGHTGREY] = K_COLOR_RGB(0x38,0x7A,0x21), //0xff387a21,
          [GB_COL_DARKGREY]  = K_COLOR_RGB(0x25,0x51,0x16), //0xff255116,
          [GB_COL_BLACK]     = K_COLOR_RGB(0x12,0x28,0x0b), //0xff12280b,
     };

     for (i = 0; i < GB_LCD_WIDTH; i++) {
          GBC_screenBuffer[(ly) * GB_LCD_WIDTH + i] = col_map[line[i].dmg_color];
     }
}

uint8_t gb_sdl_5_to_8bits(uint8_t v) {
     return (v << 3) | (v >> 2);
}

k_color gb_sdl_gbc_to_xrgb8888(uint16_t c) {
     uint8_t r = c & 0x1f;
     uint8_t g = (c >> 5) & 0x1f;
     uint8_t b = (c >> 10) & 0x1f;

     /* Extend from 5 to 8 bits */
     r = gb_sdl_5_to_8bits(r);
     g = gb_sdl_5_to_8bits(g);
     b = gb_sdl_5_to_8bits(b);

     k_color toReturn = K_COLOR_RGB(r,g,b);
     return toReturn;
}

void gb_sdl_draw_line_gbc(struct gb *gb, unsigned ly, union gb_gpu_color line[GB_LCD_WIDTH]) {

     //Only render 1/2 of the frames since the GB wants to run at 60FPS
     if(IsGBCSkippingThisFrame()) return;

     unsigned i;
     unsigned x;
     unsigned y;

     for (i = 0; i < GB_LCD_WIDTH; i++) {
          uint16_t c = line[i].gbc_color;
          GBC_screenBuffer[(ly) * GB_LCD_WIDTH + i] = gb_sdl_gbc_to_xrgb8888(c);   
     }
}


void gb_sdl_flip(struct gb *gb) {

     //Frame at GBC_screenBuffer ready
     if(IsGBCSkippingThisFrame()){ //Only render 1/2 of the frames since the GB wants to run at 60FPS
          frameCountToSkip--;
          //k_EndScreenUpdate(false,true);
          return;
     }

	//uint32_t startTime = k_GetTimeMS();
     
     k_BeginScreenUpdate();   

     if(gbc_frameCount < 5 || true){ //Clear the screen the first 5 frames, because we are using double buffer and the screenbuffer is not saved on checkpoints
          k_draw_Clear(K_COLOR_WHITE);
     }

//Upscaling algorithm adapted and optimized from https://tech-algorithm.com/articles/nearest-neighbor-image-scaling/
#ifdef GBC_UPSCALE
     int w1 = GB_LCD_WIDTH;
     int h1 = GB_LCD_HEIGHT;
     int w2 = 266;//SCREEN_WIDTH_REAL;
     int h2 = SCREEN_HEIGHT_REAL;

     int x_ratio = (int)((w1<<16)/w2) +1;
     int y_ratio = (int)((h1<<16)/h2) +1;

     k_color lineBuffer[w2];

     int old_y2 = -1;
     int x2, y2 ;
     for (int i=0;i<h2;i++) {
          y2 = ((i*y_ratio)>>16);

          if(old_y2 != y2){
               for (int j=0;j<w2;j++) {
                    x2 = ((j*x_ratio)>>16) ;
                    lineBuffer[j] = GBC_screenBuffer[(y2*w1)+x2];                    
               }
               old_y2 = y2;
          }          
          Screen_IMPL_SetLine(i,lineBuffer,27,w2);
     }
#else    	
	for(int i=0;i<GB_LCD_HEIGHT;i++){
		int total = 0;
          Screen_IMPL_SetLine(i,&GBC_screenBuffer[i*GB_LCD_WIDTH],0,GB_LCD_WIDTH);
	} 
#endif
	k_EndScreenUpdate(true,true);

	//int32_t stopTime = k_GetTimeMS();
	//LOG_I("Time: %d",stopTime-startTime);

     frameCountToSkip = GB_SKIP_FREQUENCY;

     gbc_frameCount++;

	if(k_GetSettingBool("/Other/GBC/Slow down time when cranking", false) && k_IsBeingCranked()){
		DelayMS(16);
	}
     
}