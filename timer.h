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

#ifndef TIMER_H
#define TIMER_H

#include "cpu.h"



extern void timer_pause_waiting_end_frame(void);
extern void timer_check_interrupt(void);
extern void timer_reset(void);
extern void start_timer_thread(void);

extern int timer_sleep_machine;
extern int original_timer_sleep_machine;
extern void timer_sleep(int milisec);

extern int contador_segundo;

extern int timer_condicion_top_speed(void);
extern int top_speed_real_frames;
extern z80_bit top_speed_timer;

extern z80_bit interrupcion_fifty_generada;

extern int timer_get_uptime_seconds(void);

extern void timer_get_texto_time(struct timeval *tv, char *texto);

extern int timer_get_worked_time(void);


#endif
