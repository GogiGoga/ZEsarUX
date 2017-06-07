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

z80_bit ds1307_last_clock_bit={0};
z80_bit ds1307_last_data_bit={0};

z80_byte ds1307_last_data_byte;

//Start sequence: clock 1, data 1 , data 0, clock 0
int ds1307_start_sequence_index=0;
//0= no recibido primer clock 1, ni nada
//1= recibido primer clock 1
//2= recibido data 1
//3= recibido data 0
//4= recibido clock 0 -> inicializado

z80_bit ds1307_initialized={0};

z80_byte ds1307_received_data_bits=0;


//Estado de recepcion de comandos.
//0 no recibido comando
//1 recibido comando (D0H o D1H)
//2 recibido numero registro (0..3f)
int ds_1307_received_command_state=0;

z80_byte ds_1307_received_command=0; //Ultimo comando recibido, probablemente D0 o D1
z80_byte ds_1307_received_register_number=0; //Ultimo registro recibido, probablemente entre 0 y 3f

//numero de bit enviando a una operacion de lectura
z80_byte ds13072_bitnumber_read=128;


z80_byte ds1307_registers[64];


z80_byte ds1307_get_port_clock(void)
{
	return 0;
}

z80_byte ds1307_get_port_data(void)
{
	z80_byte return_value;

	if (ds1307_registers[ds_1307_received_register_number]&ds13072_bitnumber_read) return_value=1;
	return_value=0;

	ds13072_bitnumber_read=ds13072_bitnumber_read>>1;
	if (ds13072_bitnumber_read==0) ds13072_bitnumber_read=128;

	printf ("Returning value %d register %d final_mask %d\n",return_value,ds_1307_received_register_number,ds13072_bitnumber_read);
	return return_value;
}

void ds1307_write_port_data(z80_byte value)
{
	printf ("Write ds1307 data port value:  %d bit 0: %d\n",value,value&1);
	ds1307_last_data_bit.v=value&1;

	if (value&1) {
		if (ds1307_start_sequence_index==1) ds1307_start_sequence_index=2;
		else ds1307_start_sequence_index=0; //Resetear secuencia
        }

        else {
                if (ds1307_start_sequence_index==2) {
                        ds1307_start_sequence_index=3;
		}
		else ds1307_start_sequence_index=0; //Resetear secuencia
	}
	printf ("start sequence index: %d\n",ds1307_start_sequence_index);

	//Si esta inicializado, guardar valores
	if (ds1307_initialized.v) {
		ds1307_last_data_byte = ds1307_last_data_byte << 1;
		ds1307_last_data_byte &=(255-1);
		ds1307_last_data_byte |= (value&1);

		ds1307_received_data_bits++;

		if (ds1307_received_data_bits==8) {
			printf ("---Received byte: %02XH\n",ds1307_last_data_byte);

			if (ds_1307_received_command_state==0) {
				ds_1307_received_command_state=1;
				printf ("---Received command: %02XH\n",ds1307_last_data_byte);
				ds_1307_received_command=ds1307_last_data_byte;

				if (ds1307_last_data_byte==0xD0) {
					printf ("Command D0. Read data\n");
				}

	                        if (ds1307_last_data_byte==0xD1) {
        	                        printf ("Command D1. Write data\n");
                	        }
			}

			else if (ds_1307_received_command_state==0) {
				ds_1307_received_command_state=0;
				printf ("---Received register number: %02XH\n",ds1307_last_data_byte);
				ds_1307_received_register_number=ds1307_last_data_byte;

				if (ds_1307_received_command==0xD0) {
					//Llenar array registros

					//temp. ds1307_registers
					ds1307_registers[1]=0x56; //56 minutos
				}
                        }

		}

//
//int ds_1307_received_command_state=0;
//z80_byte ds_1307_received_command=0; //Ultimo comando recibido, probablemente D0 o D1
//z80_byte ds_1307_received_register_number=0; //Ultimo registro recibido, probablemente entre 0 y 3f

			
	}

}

void ds1307_write_port_clock(z80_byte value)
{
	printf ("Write ds1307 clock port value: %d bit 0: %d\n",value,value&1);
	ds1307_last_clock_bit.v=value&1;
	if (value&1) {
		if (ds1307_start_sequence_index==0) ds1307_start_sequence_index=1;
		else ds1307_start_sequence_index=0; //Resetear secuencia
	}

	else {
		if (ds1307_start_sequence_index==3) {
			ds1307_start_sequence_index=4;
			printf ("Recibida secuencia inicializacion");
			ds1307_initialized.v=1;
			ds1307_received_data_bits=0;
		}
		else ds1307_start_sequence_index=0; //Resetear secuencia
	}

	printf ("start sequence index: %d\n",ds1307_start_sequence_index);
}


