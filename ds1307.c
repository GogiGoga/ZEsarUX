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


//numero de bit enviando a una operacion de lectura
z80_byte ds13072_bitnumber_read=128;


z80_byte ds1307_registers[64];

z80_byte ds_1307_received_register_number;

int temp_conta=0;

int ds1307_sending_data_from_speccy=1;

int ds1307_sending_data_status=0;
//0=Preparado para recibir comando
//1=Preparado para recibir registro

int ds1307_sending_data_num_bits; //Entre 0 y 7 para bits y 8 para ack
z80_byte ds1307_last_command_received;//Normalmente D0 o D1
z80_byte ds1307_last_register_received; //Normalmente entre 0 y 63

z80_byte ds1307_last_command_read_mask=128;

z80_byte ds1307_get_register(z80_byte index)
{
	index=index&63;

	if (index<8) {
							//temp. ds1307_registers
							ds1307_registers[0]=0x33; //33 segundos
							ds1307_registers[1]=0x56; //56 minutos
							ds1307_registers[2]=0x09; //09 horas

							//dia semana, dia, mes, anyo
							ds1307_registers[3]=0x03;
							ds1307_registers[4]=0x18;
							ds1307_registers[5]=0x09;
							ds1307_registers[6]=0x17;

	}

	return ds1307_registers[index];


						//}
}

z80_byte ds1307_get_port_clock(void)
{
	return 0;
}

z80_byte ds1307_get_port_data(void)
{

		z80_byte value=ds1307_get_register(ds1307_last_register_received) & ds1307_last_command_read_mask;
		if (value) value=1;
		else value=0;

		if (ds1307_last_command_read_mask==128) printf ("Returning first bit of register %d value %02XH",
			ds1307_last_register_received&63,ds1307_registers[ds1307_last_register_received&63]);

		ds1307_last_command_read_mask=ds1307_last_command_read_mask>>1;

		//Siguiente byte
		if (ds1307_last_command_read_mask==0) {
			ds1307_last_command_read_mask=128;
			ds1307_last_register_received++;
		}

	printf ("%d Returning value %d register %d final_mask %d\n",temp_conta++,value,ds1307_last_register_received,ds1307_last_command_read_mask);
	return value;
}

void ds1307_write_port_data(z80_byte value)
{
	printf ("%d Write ds1307 data port value:  %d bit 0: %d\n",temp_conta++,value,value&1);


	if (ds1307_last_clock_bit.v) {
		if (ds1307_last_data_bit.v==1 && (value&1)==0) {
      printf ("#########Recibida secuencia inicializacion\n");
			ds1307_sending_data_from_speccy=1;
			ds1307_sending_data_status=0;
			ds1307_sending_data_num_bits=0;
			ds1307_last_command_read_mask=128;

		}

		if (ds1307_last_data_bit.v==0 && (value&1)==1) {
			printf ("#########Recibida secuencia stop\n");
			ds1307_sending_data_from_speccy=1;
			ds1307_sending_data_status=0;
			ds1307_sending_data_num_bits=0;
			ds1307_last_command_read_mask=128;

    }

		ds1307_last_data_bit.v=value&1;
		return;
	}



	ds1307_last_data_bit.v=value&1;
	if (ds1307_sending_data_from_speccy) {

		//Recibiendo comando
		if (ds1307_sending_data_status==0) {
			printf ("Receiving data command\n");
			ds1307_sending_data_num_bits++;
			if (ds1307_sending_data_num_bits<=8) {
				ds1307_last_command_received=ds1307_last_command_received<<1;
				ds1307_last_command_received &=(255-1);
				ds1307_last_command_received|=value&1;
			}

			//Es ack. ignorar y cambiar estado
			if (ds1307_sending_data_num_bits==9) {
				printf ("Received ACK\n");
				printf ("Command received: %02XH\n",ds1307_last_command_received);
				ds1307_sending_data_status++;

				//Si el comando es D0, envia datos. Si es D1, solo recibe y por tanto ignoramos escrituras en D
				if (ds1307_last_command_received==0xD0) ds1307_sending_data_from_speccy=1;
				else ds1307_sending_data_from_speccy=0;

				ds1307_sending_data_num_bits=0;
			}

		}

		//Recibiendo registro
		else if (ds1307_sending_data_status==1) {
			printf ("Receiving data register\n");
			ds1307_sending_data_num_bits++;
			if (ds1307_sending_data_num_bits<=8) {
				ds1307_last_register_received=ds1307_last_register_received<<1;
				ds1307_last_register_received &=(255-1);
				ds1307_last_register_received|=value&1;
			}

			//Es ack. ignorar y cambiar estado
			if (ds1307_sending_data_num_bits==9) {
				printf ("Received ACK\n");
				printf ("Register received: %02XH\n",ds1307_last_register_received);
				ds1307_sending_data_status++;
				ds1307_sending_data_num_bits=0;
			}

		}
	}

	else {
		printf ("Ignoring write on data\n");
	}




}

void ds1307_write_port_clock(z80_byte value)
{
	printf ("%d Write ds1307 clock port value: %d bit 0: %d\n",temp_conta++,value,value&1);
	ds1307_last_clock_bit.v=value&1;

}
