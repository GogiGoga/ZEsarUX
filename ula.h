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

#ifndef ULA_H
#define ULA_H

#include "cpu.h"

extern z80_byte out_254_original_value;
extern z80_byte out_254;
extern z80_bit keyboard_issue2;

extern z80_byte last_ula_attribute;
extern z80_byte last_ula_pixel;

extern z80_byte contador_parpadeo;
extern z80_bit estado_parpadeo;


extern z80_bit ula_late_timings;

extern z80_bit pentagon_timing;

extern void ula_disable_pentagon_timing(void);
extern void ula_enable_pentagon_timing(void);

extern z80_bit ula_disabled_ram_paging;
extern z80_bit ula_disabled_rom_paging;

extern z80_bit ula_im2_slow;

extern void ula_enable_pentagon_timing_no_common(void);


//#define HARDWARE_DEBUG_PORT_ASCII 12291
//#define HARDWARE_DEBUG_PORT_NUMBER 12295
extern z80_bit hardware_debug_port;

//Usamos puertos que estaban reservados para Spectrastik pero ese dispositivo no existe ya
#define ZESARUX_ZXI_PORT_REGISTER 0xCF3B
#define ZESARUX_ZXI_PORT_DATA     0xDF3B

extern z80_byte zesarux_zxi_last_register;

extern z80_byte zesarux_zxi_registers_array[];



extern z80_byte zesarux_zxi_read_last_register(void);
extern void zesarux_zxi_write_last_register(z80_byte value);
extern void zesarux_zxi_write_register_value(z80_byte value);
extern z80_byte zesarux_zxi_read_register_value(void);

extern void generate_nmi(void);

#endif
