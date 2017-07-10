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

#ifndef TBBLUE_H
#define TBBLUE_H

#include "cpu.h"

extern z80_byte *tbblue_ram_memory_pages[];

extern z80_byte *tbblue_rom_memory_pages[];

extern z80_byte *tbblue_memory_paged[];

//extern z80_byte tbblue_config1;
//extern z80_byte tbblue_config2;
//extern z80_byte tbblue_port_24df;

extern void tbblue_out_port(z80_int port,z80_byte value);

extern void tbblue_set_memory_pages(void);

extern void tbblue_init_memory_tables(void);

extern void tbblue_hard_reset(void);

extern void tbblue_reset(void);

extern z80_byte *tbblue_fpga_rom;

extern z80_bit tbblue_low_segment_writable;

extern z80_bit tbblue_bootrom;

//extern z80_byte tbblue_read_port_24d5(void);

//extern z80_byte tbblue_read_port_24d5_index;

extern void tbblue_set_timing_48k(void);

//extern void tbblue_set_emulator_setting_timing(void);

/*
243B - Set register #
253B - Set/read value
*/

#define TBBLUE_REGISTER_PORT 0x243b
#define TBBLUE_VALUE_PORT 0x253b
#define TBBLUE_LAYER2_PORT 0x123B

#define TBBLUE_SPRITE_INDEX_PORT 0x303B
#define TBBLUE_SPRITE_PALETTE_PORT 0x53
#define TBBLUE_SPRITE_PATTERN_PORT 0x55
#define TBBLUE_SPRITE_SPRITE_PORT 0x57




#define MAX_SPRITES_PER_LINE 12

#define TBBLUE_SPRITE_BORDER 32

#define MAX_X_SPRITE_LINE (TBBLUE_SPRITE_BORDER+256+TBBLUE_SPRITE_BORDER)


extern z80_byte tbblue_registers[];

extern z80_byte tbblue_last_register;

extern z80_byte tbblue_get_value_port(void);
extern void tbblue_set_register_port(z80_byte value);
extern void tbblue_set_value_port(z80_byte value);
extern z80_byte tbblue_get_value_port_register(z80_byte registro);

extern void tbsprite_do_overlay(void);

#define TBBLUE_MAX_PATTERNS 64
#define TBBLUE_MAX_SPRITES 64
#define TBBLUE_TRANSPARENT_COLOR 0xE3

#define TBBLUE_SPRITE_HEIGHT 16
#define TBBLUE_SPRITE_WIDTH 16

extern z80_byte tbsprite_patterns[TBBLUE_MAX_PATTERNS][256];
extern z80_byte tbsprite_palette[];
extern z80_byte tbsprite_sprites[TBBLUE_MAX_SPRITES][4];

extern void tbblue_out_port_sprite_index(z80_byte value);
extern void tbblue_out_sprite_palette(z80_byte value);
extern void tbblue_out_sprite_pattern(z80_byte value);
extern void tbblue_out_sprite_sprite(z80_byte value);
extern z80_byte tbblue_get_port_sprite_index(void);

extern z80_byte tbblue_port_123b;
extern int tbblue_write_on_layer2(void);
extern int tbblue_get_offset_start_layer2(void);
extern z80_byte tbblue_get_port_layer2_value(void);
extern void tbblue_out_port_layer2_value(z80_byte value);
extern int tbblue_is_active_layer2(void);


#endif
