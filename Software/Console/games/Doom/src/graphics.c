/**
 *  Display utility functions for Doom Port to nRF52840 MCU.
 *  It also contains the interrupt-assisted DMA update of the display
 *  and double buffer display data structure.
 *
 *  Copyright (C) 2021 by Nicola Wrachien (next-hack in the comments)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 *  DESCRIPTION:
 *  Interrupt assisted DMA-based display update engine, with double buffering 
 *  256-color functions to print general purpose texts.
 *  
 */

#include "Helpers/Misc.h"
#include "graphics.h"
#include "Fonts/font8x8.h"
#include "printf.h"
#include <string.h>
#include "Helpers/Screen/Screen.h"
#include "Kernel_Draw.h"
#include "Kernel.h"
//
void Screen_IMPL_SetLine(int y, k_color* data, int lineOffset, int pixelToSet);

#define XSTR(s) STR(s)
#define STR(s) #s
// note! LDR instructions are pipelined!
#define CONVERT_4_PIX(START_PIX)  "LDRB %[pix0], [%[buff], #0 +" XSTR(START_PIX) "]\n\t" \
                                  "LDRB %[pix1], [%[buff], #1 +" XSTR(START_PIX) "]\n\t" \
                                  "LDRB %[pix2], [%[buff], #2 +" XSTR(START_PIX) "]\n\t" \
                                  "LDRB %[pix3], [%[buff], #3 +" XSTR(START_PIX) "]\n\t" \
                                  "LDRH %[pix0], [%[palette], %[pix0], LSL #1]\n\t" \
                                  "LDRH %[pix1], [%[palette], %[pix1], LSL #1]\n\t" \
                                  "LDRH %[pix2], [%[palette], %[pix2], LSL #1]\n\t" \
                                  "LDRH %[pix3], [%[palette], %[pix3], LSL #1]\n\t" \
                                  "ORR %[pix0], %[pix0], %[pix1], LSL #16\n\t" \
                                  "STR.w %[pix0], [%[dmabuff], #0 + " XSTR(2 * START_PIX) "]\n\t" \
                                  "ORR %[pix2], %[pix2], %[pix3], LSL #16\n\t" \
                                  "STR.w %[pix2], [%[dmabuff], #4 + " XSTR(2 * START_PIX) "]\n\t"


CHECKPOINT_EXCLUDE_BSS displayData_t displayData; //Framebuffer for doom. This can be excluded from checkpointing if checkpointing is only performed at the end of each frame
void initGraphics()
{
    memset(&displayData, 0, sizeof(displayData));
}

void startDisplayRefresh()
{    
     uint32_t time = k_GetTimeMS();
    k_BeginScreenUpdate();

   // k_draw_Clear(K_COLOR_WHITE);
    Screen_SendUpdateFromDOOM(displayData.displayFrameBuffer);  
    k_EndScreenUpdate(true, true);
   // LOG_I("Time %d",k_GetTimeMS()-time);   
}


#define DOOM_WIDTH 320
k_color doomLineBuffer[DOOM_WIDTH];

#pragma GCC optimize("Ofast")
void Screen_SendUpdateFromDOOM(uint8_t* doomFramePtr){
    for(int y=0;y<SCREEN_HEIGHT;y++){
        fillNextDmaLineBufferTEMP(doomLineBuffer,doomFramePtr,DOOM_WIDTH); 
       // for(int i=0;i<240;i++){
        //    UART_OUT("%02X%02X%02X ",doomLineBuffer[i].r,doomLineBuffer[i].g,doomLineBuffer[i].b);      
        //}
       // UART_OUT("\n");
        
        doomFramePtr+=DOOM_WIDTH;   
        Screen_IMPL_SetLine(y,doomLineBuffer,0,DOOM_WIDTH);       
    }  
}

#pragma GCC optimize ("Ofast")  // we need to compile this code to be as fast as possible.
inline void fillNextDmaLineBufferTEMP(k_color * pLineBuffer,uint8_t* pixelsToSend,uint32_t count)
{     
    volatile k_color * pPalette = (volatile k_color *)displayData.pPalette;

    for (int i = 0; i < count; i++)
    {
        *pLineBuffer++ = pPalette[*pixelsToSend++];
    }
}

