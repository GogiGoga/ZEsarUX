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
//int ds1307_start_sequence_index=0;
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

/*
START  data  transfer:  
A  change  in  the  state  of  the  data  line,  from  HIGH  to  LOW,  while  the  clock  is  HIGH,  
defines a START condition.
STOP data transfer: 
A change in the state of the data line, from LOW to HIGH, while the clock line is HIGH, 
defines the STOP condition.
*/

//Master: speccy
//Slave: este chip

//Pendiente recibir ack desde speccy
int ds13072_pending_ack_master_to_slave=0;

//Pendiente enviar ack hacia speccy
z80_bit ds13072_pending_ack_slave_to_master={0};

//temp
int enviando_registros=0;

/*
Ack y NACK
SEND_ACK:	
	xor a  ;a=0	
	
SEND_ACK_NACK:
	
	CALL SDA
	
	call PULSE_CLOCK
	
;	free the data line
	CALL SDA1

	ret


*/


z80_byte ds1307_get_port_clock(void)
{
	return 0;
}

z80_byte ds1307_get_port_data(void)
{
	z80_byte return_value;

	if (ds13072_pending_ack_slave_to_master.v) {
		printf ("Sending ACK\n");
		ds13072_pending_ack_slave_to_master.v=0;
		return 1;
	}


	if (ds13072_bitnumber_read==128) printf ("Sending first data bit of register %d value %d\n",ds_1307_received_register_number&0x3f,
		ds1307_registers[ds_1307_received_register_number&0x3f]);


	if (ds1307_registers[ds_1307_received_register_number&0x3f] & ds13072_bitnumber_read) return_value=1;
	else return_value=0;

	ds13072_bitnumber_read=ds13072_bitnumber_read>>1;
	if (ds13072_bitnumber_read==0) {
		ds13072_bitnumber_read=128;
		ds_1307_received_register_number++;
		ds13072_pending_ack_master_to_slave=1;
	}

	printf ("-----Returning value %d register %d final_mask %d\n",return_value,ds_1307_received_register_number,ds13072_bitnumber_read);
	return return_value;
}

void ds1307_write_port_data(z80_byte value)
{
	printf ("Write ds1307 data port value:  %d bit 0: %d\n",value,value&1);

/*
A  change  in  the  state  of  the  data  line,  from  HIGH  to  LOW,  while  the  clock  is  HIGH,
defines a START condition.
*/


/*
STOP data transfer:
A change in the state of the data line, from LOW to HIGH, while the clock line is HIGH,
defines the STOP condition.
*/

	if (ds1307_last_clock_bit.v) {
		if (ds1307_last_data_bit.v==1 && (value&1)==0) {
                        printf ("#########Recibida secuencia inicializacion\n");
                        ds1307_initialized.v=1;
                        ds1307_received_data_bits=0;	
			ds_1307_received_command_state=0;
			enviando_registros=0;
		}

		if (ds1307_last_data_bit.v==0 && (value&1)==1) {
			printf ("#########Recibida secuencia stop\n");
			//ds1307_initialized.v=0;
			//ds1307_received_data_bits=0;
			enviando_registros=0;
                }

		ds1307_last_data_bit.v=value&1;
		return;
	}

	if (ds13072_pending_ack_master_to_slave) {
		if (ds13072_pending_ack_master_to_slave==1) printf ("Received ACK 1\n");
		if (ds13072_pending_ack_master_to_slave==2) printf ("Received ACK 2\n");
		ds13072_pending_ack_master_to_slave++;
		if (ds13072_pending_ack_master_to_slave==3) ds13072_pending_ack_master_to_slave=0;
		return;
	}

	ds1307_last_data_bit.v=value&1;


	//printf ("start sequence index: %d\n",ds1307_start_sequence_index);

	//Si esta inicializado, guardar valores
	if (ds1307_initialized.v) {
		if (1==0) {
			//Al final de cada 8 bits se envia un bit a 1
			//Ignorar ese bit
			//ds1307_received_data_bits=0;
			//printf ("---Received final ignored bit\n");
		}

		else {

			if (enviando_registros) return;


			ds1307_last_data_byte = ds1307_last_data_byte << 1;
			ds1307_last_data_byte &=(255-1);
			ds1307_last_data_byte |= (value&1);

			ds1307_received_data_bits++;

		

			if (ds1307_received_data_bits==8) {
				printf ("---Received byte: %02XH\n",ds1307_last_data_byte);
				ds1307_received_data_bits=0;

				ds13072_pending_ack_slave_to_master.v=1;

				//Parece que se envia un bit a 1 al final de cada secuencia de 8 bits
				ds13072_pending_ack_master_to_slave=1; 
/*
Acknowledge: 
Each  receiving  device,  when  addressed,  is  obliged  to  generate  an  acknowledge  after  the  
reception  of  each  byte.  The  master  device  must  generate  an  extra  clock  pulse  which  is  associated  with  this  
acknowledge bit
*/

				if (ds_1307_received_command_state==0) {
					ds_1307_received_command_state=1;
					printf ("---Received command: %02XH\n",ds1307_last_data_byte);
					ds_1307_received_command=ds1307_last_data_byte;
					ds13072_pending_ack_master_to_slave=2; //Solo 1 bit de ack

					if (ds1307_last_data_byte==0xD0) {
						printf ("Command D0. Write data\n");
					}

	                	        if (ds1307_last_data_byte==0xD1) {
        	                	        printf ("Command D1. Read data\n");
						enviando_registros=1;
	                	        }
				}

				else if (ds_1307_received_command_state==1) {
					ds_1307_received_command_state=2;
					printf ("---Received register number: %02XH\n",ds1307_last_data_byte);
					ds_1307_received_register_number=ds1307_last_data_byte;
					ds13072_pending_ack_master_to_slave=2; //Solo 1 bit de ack

					//if (ds_1307_received_command==0xD0) {
						//Llenar array registros
						//printf ("---Received command D0 and register number\n");
						printf ("---Filling registers array\n");

						//temp. ds1307_registers
						ds1307_registers[0]=0x33; //33 segundos
						ds1307_registers[1]=0x56; //56 minutos
						ds1307_registers[2]=0x09; //09 horas

						//dia semana, dia, mes, anyo
						ds1307_registers[3]=0x03;
						ds1307_registers[4]=0x18;
						ds1307_registers[5]=0x09;
						ds1307_registers[6]=0x17;

						
					//}
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

}


