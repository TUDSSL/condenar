// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2008 VBA-M development team

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

#ifndef __VBA_INPUT_DRIVER_H__
#define __VBA_INPUT_DRIVER_H__

#include "glib.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read the state of an emulated joypad
 * @return Joypad state
 */
guint32 read_joypad();
#ifdef __cplusplus
}
#endif


/**
 * Compute the motion sensor X and Y values
 */
void update_motion_sensor();


#endif // __VBA_INPUT_DRIVER_H__
