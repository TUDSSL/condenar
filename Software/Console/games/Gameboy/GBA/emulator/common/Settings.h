// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2005-2006 Forgotten and the VBA development team

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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef VBAM_SDL_SETTINGS_H_
#define VBAM_SDL_SETTINGS_H_

#include <glib.h>


/**
 * Available log channels
 */
typedef enum {
	LOG_SWI = 1,
	LOG_UNALIGNED_MEMORY,
	LOG_ILLEGAL_WRITE,
	LOG_ILLEGAL_READ,
	LOG_DMA0,
	LOG_DMA1,
	LOG_DMA2,
	LOG_DMA3,
	LOG_UNDEFINED,
	LOG_AGBPRINT,
	LOG_SOUNDOUTPUT,
} LogChannel;

/**
 * @param channel log channel
 * @return TRUE if the specified log channel is enabled
 */

#ifdef __cplusplus
extern "C" {
#endif

gboolean settings_log_channel_enabled(LogChannel channel);
#ifdef __cplusplus
}
#endif


#endif /* VBAM_SDL_SETTINGS_H_ */
