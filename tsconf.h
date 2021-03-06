/*
    ZEsarUX  ZX Second-Emulator And Released for UniX
    Copyright (C) 2013 Cesar Hernandez Bano

    This file is part of ZEsarUX.

    ZEsarUX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef TSCONF_H
#define TSCONF_H

#include "cpu.h"


#define TSCONF_LEFT_BORDER_NO_ZOOM 0
#define TSCONF_TOP_BORDER_NO_ZOOM 0

#define TSCONF_LEFT_BORDER TSCONF_LEFT_BORDER_NO_ZOOM*zoom_x
#define TSCONF_TOP_BORDER TSCONF_TOP_BORDER_NO_ZOOM*zoom_y

#define TSCONF_DISPLAY_WIDTH 720
#define TSCONF_DISPLAY_HEIGHT 576

#define TSCONF_FMAPS_SIZE 1024
#define TSCONF_ROM_PAGES 32
#define TSCONF_RAM_PAGES 256

extern z80_byte tsconf_last_port_eff7;
extern z80_byte tsconf_last_port_dff7;
extern z80_byte tsconf_nvram[];

extern void tsconf_write_af_port(z80_byte puerto_h,z80_byte value);
extern z80_byte tsconf_get_af_port(z80_byte index);
extern void tsconf_init_memory_tables(void);
extern void tsconf_set_memory_pages(void);
extern z80_byte *tsconf_memory_paged[];
extern z80_byte *tsconf_rom_mem_table[];
extern z80_byte *tsconf_ram_mem_table[];
extern z80_byte tsconf_af_ports[];

extern z80_byte tsconf_fmaps[];

extern int temp_tsconf_in_system_rom_flag;

extern void tsconf_reset_cpu(void);
extern void tsconf_hard_reset(void);

extern z80_byte tsconf_get_text_font_page(void);

extern int tsconf_current_pixel_width;
extern int tsconf_current_pixel_height;
extern int tsconf_current_border_width;
extern int tsconf_current_border_height;

extern z80_byte tsconf_get_video_mode_display(void);

extern z80_byte tsconf_get_memconfig(void);

extern z80_byte tsconf_get_vram_page(void);

extern z80_int tsconf_return_cram_color(z80_byte color);

extern z80_byte tsconf_return_cram_palette_offset(void);

extern void tsconf_set_sizes_display(void);

extern z80_byte tsconf_get_rom_bank(void);

extern z80_bit tsconf_with_vdac;

extern z80_byte tsconf_rgb_5_to_8(z80_byte color);


#endif
