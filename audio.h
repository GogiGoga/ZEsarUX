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

#ifndef AUDIO_H
#define AUDIO_H

#include "cpu.h"

//lineas de cada pantalla. al final de cada linea se guarda el bit a enviar al altavoz
//#define AUDIO_BUFFER_SIZE 312

//Dice cuantos frames enteros de pantalla hay que esperar antes de enviar el audio
//en ZXSpectr usamos buffers de 10 frames
//pero parece que aqui como el sincronismo no es tan bueno, no se puede hacer tanto
#define FRAMES_VECES_BUFFER_AUDIO 5
//#define FRAMES_VECES_BUFFER_AUDIO 10

#define AUDIO_BUFFER_SIZE (312*FRAMES_VECES_BUFFER_AUDIO)

//#define FRECUENCIA_SONIDO AUDIO_BUFFER_SIZE*50

//Frecuencia normal constante sin tener en cuenta velocidad cpu. Se usa en chip AY como referencia y en pocos sitios mas
#define FRECUENCIA_CONSTANTE_NORMAL_SONIDO (312*50)

//temp
//#define FRECUENCIA_SONIDO 312*50

#define FRECUENCIA_SONIDO frecuencia_sonido_variable

//Frecuencia teniendo en cuenta la velocidad de la cpu
extern int frecuencia_sonido_variable;



//duracion en ms de un buffer de sonido, normalmente  (1000.0/((FRECUENCIA_SONIDO)/AUDIO_BUFFER_SIZE))
//usado en audiodsp con phtreads
#define AUDIO_MS_DURACION 100


#define AMPLITUD_BEEPER 50
#define AMPLITUD_BEEPER_GRABACION_ZX8081 25
#define AMPLITUD_TAPE 2

extern int amplitud_speaker_actual_zx8081;

extern int (*audio_init) (void);
extern int (*audio_thread_finish) (void);
extern void (*audio_end) (void);
extern void (*audio_send_frame) (char *buffer);
extern void (*audio_get_buffer_info) (int *buffer_size,int *current_size);
extern char *audio_buffer;
extern char *audio_buffer_playback;

extern z80_bit audio_noreset_audiobuffer_full;

extern char *audio_driver_name;

extern char *audio_buffer_one;
extern char *audio_buffer_two;
extern char audio_buffer_oneandtwo[AUDIO_BUFFER_SIZE*2];


extern int audio_buffer_indice;
extern z80_bit audio_buffer_switch;
extern void set_active_audio_buffer(void);

extern z80_byte ultimo_altavoz;
extern z80_bit bit_salida_sonido_zx8081;

extern char value_beeper;


extern z80_bit interrupt_finish_sound;

//5 segundos hasta detectar silencio
#define SILENCE_DETECTION_MAX 50*5


extern int silence_detection_counter;
extern int beeper_silence_detection_counter;

//aofile
extern char *aofilename;
extern void init_aofile(void);
extern void close_aofile(void);
extern void aofile_send_frame(char *buffer);
extern z80_bit aofile_inserted;

#define AOFILE_TYPE_RAW 0
#define AOFILE_TYPE_WAV 1
extern int aofile_type;


extern z80_bit audio_playing;
extern void envio_audio(void);

//#define BEEPER_ARRAY_LENGTH 228
#define MAX_BEEPER_ARRAY_LENGTH MAX_STATES_LINE

#define CURRENT_BEEPER_ARRAY_LENGTH MAX_CURRENT_STATES_LINE

extern int buffer_beeper[];
extern int beeper_real_enabled;

extern void beeper_new_line(void);
extern char get_value_beeper_sum_array(void);
extern void set_value_beeper_on_array(char value);

extern int audiovolume;

extern z80_bit output_beep_filter_on_rom_save;

extern z80_bit output_beep_filter_alter_volume;
extern char output_beep_filter_volume;

extern int audio_adjust_volume(int valor_enviar);

struct s_nota_musical
{
        char nombre[4];
	int frecuencia; //aunque tiene decimales, para lo que necesitamos no hacen falta
};

typedef struct s_nota_musical nota_musical;

#define NOTAS_MUSICALES_OCTAVAS 10
#define NOTAS_MUSICALES_NOTAS_POR_OCTAVA 12
#define MAX_NOTAS_MUSICALES (NOTAS_MUSICALES_OCTAVAS*NOTAS_MUSICALES_NOTAS_POR_OCTAVA)

extern char *get_note_name(int frecuencia);
extern int set_audiodriver_null(void);
extern void fallback_audio_null(void);
extern void audio_empty_buffer(void);

extern char audio_valor_enviar_sonido;

extern int audio_ay_player_load(char *filename);
extern z80_byte *audio_ay_player_mem;
extern z80_byte ay_player_pista_actual;

extern z80_byte ay_player_total_songs(void);

extern int audio_ay_player_play_song(z80_byte song);

extern void ay_player_load_and_play(char *filename);

extern z80_bit ay_player_playing;

extern void ay_player_playing_timer(void);
extern void ay_player_next_track (void);
extern void ay_player_previous_track (void);

extern void ay_player_stop_player(void);


extern z80_int ay_song_length;
extern z80_int ay_song_length_counter;

extern char ay_player_file_song_name[];
extern char ay_player_file_author[];
extern char ay_player_file_misc[];


extern z80_bit ay_player_exit_emulator_when_finish;
extern z80_int ay_player_limit_infinite_tracks;
extern z80_int ay_player_limit_any_track;
extern z80_bit ay_player_repeat_file;

extern z80_bit ay_player_cpc_mode;

extern z80_byte audiodac_last_value_data;

extern z80_bit audiodac_enabled;

extern int audiodac_selected_type;

//extern z80_int audiodac_custom_port;

struct s_audiodac_type {
  char name[20];
  z80_int port;
};

typedef struct s_audiodac_type audiodac_type;

#define MAX_AUDIODAC_TYPES 5

extern audiodac_type audiodac_types[];

extern z80_bit beeper_enabled;

extern void audiodac_mix(void);


#endif
