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
#ifndef SRC_GRAPHICS_H_
#define SRC_GRAPHICS_H_
#include <stdbool.h>
#include <stdint.h>

#include "Kernel_Draw.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MAXROWS (SCREEN_HEIGHT / 8)
#define MAXCOLS (SCREEN_WIDTH / 8)

typedef struct
{
    volatile k_color* pPalette;
    uint8_t displayFrameBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
} displayData_t;

extern displayData_t displayData;
void displayPrintf(int x, int y, const char * format, ...);
void startDisplayRefresh();
void setDisplayPen(int color, int background);
void displayPrintln(bool update, const char * format, ...);
void initGraphics();

void fillNextDmaLineBufferTEMP(k_color * pLineBuffer,uint8_t* pixelsToSend,uint32_t count);
#endif /* SRC_GRAPHICS_H_ */