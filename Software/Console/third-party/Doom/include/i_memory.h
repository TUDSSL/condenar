/**
 *  Doom Port to the nRF52840 by Nicola Wrachien (next-hack in the comments)
 *
 *  This port is based on the excellent doomhack's GBA Doom Port,
 *  with Kippykip additions.
 *  
 *  Several data structures and functions have been optimized 
 *  to fit in only 256kB RAM of nRF52840 (GBA has 384 kB RAM). 
 *  Z-Depth Light has been restored with almost no RAM consumption!
 *  Tons of speed optimizations have been done, and the game now
 *  runs extremely fast, despite the much higher 3D resolution with
 *  respect to GBA.
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright (C) 2021 Nicola Wrachien (next-hack in the comments)
 *  on nRF52840 port.
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
 * DESCRIPTION:
 * next-hack: packed (24) and 16 bit pointers utility functions and other stuff
 */

#ifndef DOOM_INCLUDE_I_MEMORY_H_
#define DOOM_INCLUDE_I_MEMORY_H_
#include <stdint.h>
#include "Helpers/Flash.h"

#define RAM_PTR_BASE 0x20000000UL
#define EXT_FLASH_BASE MSPI_XIP_BASE_ADDRESS
#define CACHE_ADDRESS SRAM_XIP_BASE_ADDRESS
#define PACKED_MEMORY_ADDRESS0 RAM_PTR_BASE
#define PACKED_MEMORY_ADDRESS1 RAM_PTR_BASE
#define PACKED_MEMORY_ADDRESS2 EXT_FLASH_BASE
#define PACKED_MEMORY_ADDRESS3 FLASH_PTR_BASE
//
#define WAD_ADDRESS (EXT_FLASH_BASE + 4)
#define FLASH_ADDRESS CACHE_ADDRESS
#define FLASH_IMMUTABLE_REGION_ADDRESS FLASH_ADDRESS
#define FLASH_IMMUTABLE_REGION 0
#define FLASH_LEVEL_REGION 1
#define FLASH_CACHE_REGION_SIZE (1024 * 750)
#define FLASH_BLOCK_SIZE 4096

#define isOnExternalFlash(a) (((uint32_t) a & EXT_FLASH_BASE) == EXT_FLASH_BASE && false)




#endif /* DOOM_INCLUDE_I_MEMORY_H_ */
