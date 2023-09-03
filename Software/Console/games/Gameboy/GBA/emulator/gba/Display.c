// VBA-M, A Nintendo Handheld Console Emulator
// Copyright (C) 2008 VBA-M development team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "Display.h"

#include <string.h>
#include "Helpers/Flash.h"
#include "GameboyAdvance.h"
#include "Helpers/Misc.h"
#include "Kernel_Draw.h"
#include "Kernel.h"

#define GBA_INTERNAL_SCREEN_SIZE_WIDTH 240
#define GBA_INTERNAL_SCREEN_SIZE_HEIGHT 160

//static guint16 gba_frame_buffer[GBA_INTERNAL_SCREEN_SIZE_WIDTH * GBA_INTERNAL_SCREEN_SIZE_HEIGHT];
static guint16* gba_frame_buffer = (uint16_t*)((uint8_t*)SRAM_XIP_BASE_ADDRESS + 550000);

void display_init()
{
	display_clear();
}

void display_clear()
{
	memset(gba_frame_buffer, 0, GBA_INTERNAL_SCREEN_SIZE_WIDTH * GBA_INTERNAL_SCREEN_SIZE_HEIGHT * sizeof(guint16));
}

void display_draw_line(int line, guint32* src)
{
	uint16_t *dest = gba_frame_buffer + GBA_INTERNAL_SCREEN_SIZE_WIDTH * line;
	for (int x = 0; x < GBA_INTERNAL_SCREEN_SIZE_WIDTH; )
	{
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;

		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;

		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;

		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
		*dest++ = src[x++] & 0xFFFF;
	}
}

void display_draw_screen()
{
	
	//LOG_I("Starting GBA frame render...");
	uint32_t startTime = k_GetTimeMS();
    k_BeginScreenUpdate();
    k_draw_Clear(K_COLOR_WHITE);

	
	for(int i=0;i<160;i++){
		int total = 0;
		for(int x=0;x<240;x++) {
			uint16_t data = gba_frame_buffer[i*240+x];
			//uint8_t blue = (data & 0b0111110000000000) >> 10;
			//uint8_t green = (data & 0b0000001111100000) >> 5;
			//uint8_t red = data & 0b0000000000011111;

			uint8_t blue = (data & 0b0111110000000000) >> 7;
			uint8_t green = (data & 0b0000001111100000) >> 2;
			uint8_t red = (data & 0b0000000000011111) << 3;

			k_color colorToSet = K_COLOR_RGB(red,green,blue);
			k_draw_SetPixel(x,i, colorToSet);
		}
	}
	//Draw the screen here




    //k_draw_DrawTextCentered(SCREEN_WIDTH_REAL/2,SCREEN_HEIGHT_REAL/2-4,1,"App: %s", k_getCurrentApp()->name);
    
	
	
	
	
	k_EndScreenUpdate(false,false);
	//LOG_I("GBA frame render finished");

	uint32_t stopTime = k_GetTimeMS();
	LOG_I("Time: %d",stopTime-startTime);

}
