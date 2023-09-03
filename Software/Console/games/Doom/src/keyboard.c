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
 * Description: Keyboard handling functions for different types
 * (parallel, i2c and radio). For radio keyboard, this handles
 * also audio data transmission.
 */
#include "keyboard.h"
#include "Helpers/PinDefinitions.h"
#include "Helpers/Misc.h"
#include "Kernel.h"
#include "d_player.h"
#include "global_data.h"

extern bool doomBeingCranked;

void getKeys(uint8_t * keys)
{
    uint8_t buttons = 0;

    k_input_state state;
    k_GetInput(&state, true);

    buttons|=(state.buttonX) * KEY_CHGW;
    buttons|=(state.buttonB) * KEY_ALT;
    buttons|=(state.buttonA) * KEY_USE;

    if(!k_GetSettingBool("/DOOM/Use crank",false) || !k_GetSettingBool("/Energy/No input when crank enabled",false)){
        buttons|=(state.triggerLeft|state.triggerRight) * KEY_FIRE;
    }

    float joyX,joyY;
    GetJoysticks(&joyX,&joyY);
    buttons|=(state.joystickX>0.25) * KEY_RIGHT;
    buttons|=(state.joystickX<-0.25) * KEY_LEFT;
    buttons|=(state.joystickY>0.25) * KEY_UP;
    buttons|=(state.joystickY<-0.25) * KEY_DOWN;

    player_t *plyr = &_g->player;
    if(plyr->forceFire){
        buttons|=KEY_FIRE;
    }

    *keys = buttons;    


}