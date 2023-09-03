#ifndef _GB_SDL_H_
#define _GB_SDL_H_

#include "gb.h"

void gb_sdl_draw_line_dmg(struct gb *gb, unsigned ly, union gb_gpu_color line[GB_LCD_WIDTH]);
void gb_sdl_draw_line_gbc(struct gb *gb, unsigned ly, union gb_gpu_color line[GB_LCD_WIDTH]);
void gb_sdl_flip(struct gb *gb);

#endif /* _GB_SDL_H_ */
