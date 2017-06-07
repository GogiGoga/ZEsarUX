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

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>


#include "ds1307.h"
#include "cpu.h"
#include "debug.h"
#include "utils.h"
#include "operaciones.h"
#include "menu.h"


z80_byte ds1307_get_port_clock(void)
{
	return 0;
}
z80_byte ds1307_get_port_data(void)
{
	return 0;
}

void ds1307_write_port_data(z80_byte value)
{
	printf ("Write ds1307 data port value: %d bit 0: %d\n",value,value&1);
}

void ds1307_write_port_clock(z80_byte value)
{
	printf ("Write ds1307 clock port value: %d\n",value);
}


