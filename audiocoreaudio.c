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

#include <AudioUnit/AudioComponent.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/AudioHardware.h>



#include "audio.h"
#include "audiocoreaudio.h"
#include "cpu.h"
#include "debug.h"


//saudiocoreaudio_fifo_t sound_fifo;

char *buffer_actual;




/* info about the format used for writing to output unit */
static AudioStreamBasicDescription deviceFormat;

/* converts from Fuse format (signed 16 bit ints) to CoreAudio format (floats)
 */
static AudioUnit gOutputUnit;

/* Records sound writer status information */
static int audio_output_started;

void audiocoreaudio_fifo_write(char *origen,int longitud);


static
OSStatus coreaudiowrite( void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,
                         AudioBufferList *ioData );


/* get the default output device for the HAL */
int get_default_output_device(AudioDeviceID* device)
{
  OSStatus err = kAudioHardwareNoError;
  UInt32 count;

  AudioObjectPropertyAddress property_address = {
    kAudioHardwarePropertyDefaultOutputDevice,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  /* get the default output device for the HAL */
  count = sizeof( *device );
  err = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property_address,
                                    0, NULL, &count, device);
  if ( err != kAudioHardwareNoError && device != kAudioObjectUnknown ) {
    debug_printf( VERBOSE_ERR,
              "get kAudioHardwarePropertyDefaultOutputDevice error %ld",
              (long)err );
    return 1;
  }

  return 0;
}


/* get the nominal sample rate used by the supplied device */
static int
get_default_sample_rate( AudioDeviceID device, Float64 *rate )
{
  OSStatus err = kAudioHardwareNoError;
  UInt32 count;

  AudioObjectPropertyAddress property_address = {
    kAudioDevicePropertyNominalSampleRate,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  /* get the default output device for the HAL */
  count = sizeof( *rate );
  err = AudioObjectGetPropertyData( device, &property_address, 0, NULL, &count,
                                    rate);
  if ( err != kAudioHardwareNoError ) {
    debug_printf( VERBOSE_ERR,
              "get kAudioDevicePropertyNominalSampleRate error %ld",
              (long)err );
    return 1;
  }

  return 0;
}



int audiocoreaudio_init(void)
{

	debug_printf (VERBOSE_INFO,"Init CoreAudio Driver, %d Hz",FRECUENCIA_SONIDO);

	buffer_actual=audio_buffer_one;

int *freqptr;
int freqqqq;
freqptr=&freqqqq;

  OSStatus err = kAudioHardwareNoError;
  AudioDeviceID device = kAudioObjectUnknown; /* the default device */
  //int error;
  float hz;
  int sound_framesiz;

  if( get_default_output_device(&device) ) return 1;
  if( get_default_sample_rate( device, &deviceFormat.mSampleRate ) ) return 1;

  *freqptr = deviceFormat.mSampleRate;

  deviceFormat.mFormatID =  kAudioFormatLinearPCM;
  deviceFormat.mFormatFlags =  kLinearPCMFormatFlagIsSignedInteger
#ifdef WORDS_BIGENDIAN
                    | kLinearPCMFormatFlagIsBigEndian
#endif      /* #ifdef WORDS_BIGENDIAN */
                    | kLinearPCMFormatFlagIsPacked;

//inDesc.mSampleRate=rate;

//parece que esto no hace nada
deviceFormat.mSampleRate=FRECUENCIA_SONIDO;


char *stereoptr;
char pepe;
stereoptr=&pepe;

*stereoptr=0x0;

  deviceFormat.mBytesPerPacket = *stereoptr ? 4 : 2;
deviceFormat.mBytesPerPacket=1;

  deviceFormat.mFramesPerPacket = 1;
  deviceFormat.mBytesPerFrame = *stereoptr ? 4 : 2;
  deviceFormat.mBytesPerFrame = 1;

  //deviceFormat.mBitsPerChannel = 16;
  deviceFormat.mBitsPerChannel = 8;
  deviceFormat.mChannelsPerFrame = *stereoptr ? 2 : 1;
  deviceFormat.mChannelsPerFrame = 1;

  /* Open the default output unit */
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent comp = AudioComponentFindNext( NULL, &desc );
  if( comp == NULL ) {
    debug_printf( VERBOSE_ERR, "AudioComponentFindNext" );
    return 1;
  }

  err = AudioComponentInstanceNew( comp, &gOutputUnit );
  if( comp == NULL ) {
    debug_printf( VERBOSE_ERR, "AudioComponentInstanceNew=%ld", (long)err );
    return 1;
  }

  /* Set up a callback function to generate output to the output unit */
  AURenderCallbackStruct input;
  input.inputProc = coreaudiowrite;
  input.inputProcRefCon = NULL;

  err = AudioUnitSetProperty( gOutputUnit,
                              kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input,
                              0,
                              &input,
                              sizeof( input ) );
  if( err ) {
    debug_printf( VERBOSE_ERR, "AudioUnitSetProperty-CB=%ld", (long)err );
    return 1;
  }

  err = AudioUnitSetProperty( gOutputUnit,
                              kAudioUnitProperty_StreamFormat,
                              kAudioUnitScope_Input,
                              0,
                              &deviceFormat,
                              sizeof( AudioStreamBasicDescription ) );
  if( err ) {
    debug_printf( VERBOSE_ERR, "AudioUnitSetProperty-SF=%4.4s, %ld", (char*)&err,
              (long)err );
    return 1;
  }

  err = AudioUnitInitialize( gOutputUnit );
  if( err ) {
    debug_printf( VERBOSE_ERR, "AudioUnitInitialize=%ld", (long)err );
    return 1;
  }

  /* Adjust relative processor speed to deal with adjusting sound generation
     frequency against emulation speed (more flexible than adjusting generated
     sample rate) */
  //hz = (float)sound_get_effective_processor_speed() /
    //          machine_current->timings.tstates_per_frame;


  /* Amount of audio data we will accumulate before yielding back to the OS.
     Not much point having more than 100Hz playback, we probably get
     downgraded by the OS as being a hog too (unlimited Hz limits playback
     speed to about 2000% on my Mac, 100Hz allows up to 5000% for me) */


	hz=FRECUENCIA_SONIDO;

  sound_framesiz = deviceFormat.mSampleRate / hz;

  /* wait to run sound until we have some sound to play */
  audio_output_started = 0;




	//Esto debe estar al final, para que funcione correctamente desde menu, cuando se selecciona un driver, y no va, que pueda volver al anterior
	audio_driver_name="coreaudio";


  return 0;
}


void sound_lowlevel_frame( char *data, int len );

void audiocoreaudio_send_frame(char *buffer)
{

if (audio_playing.v==0) audio_playing.v=1;

//sound_lowlevel_frame(buffer,AUDIO_BUFFER_SIZE);
buffer_actual=buffer;


  if( !audio_output_started ) {
    /* Start the rendering
       The DefaultOutputUnit will do any format conversions to the format of the
       default device */
    OSStatus err ;

	err= AudioOutputUnitStart( gOutputUnit );
    if( err ) {
      debug_printf( VERBOSE_ERR, "AudioOutputUnitStart=%ld", (long)err );
      return;
    }

    audio_output_started = 1;
  }

audiocoreaudio_fifo_write(buffer,AUDIO_BUFFER_SIZE);


}

int audiocoreaudio_thread_finish(void)
{

return 0;
}

void audiocoreaudio_end(void)
{
        debug_printf (VERBOSE_INFO,"Ending coreaudio audio driver");
	audiocoreaudio_thread_finish();


	struct AURenderCallbackStruct callback;

	/* stop processing the audio unit */
	if (AudioOutputUnitStop(gOutputUnit) != noErr) {
		debug_printf (VERBOSE_ERR,"audiocoreaudio_end AudioOutputUnitStop failed");
		return;
	}

	/* Remove the input callback */
	callback.inputProc = 0;
	callback.inputProcRefCon = 0;
	if (AudioUnitSetProperty(gOutputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, sizeof(callback)) != noErr) {
		debug_printf (VERBOSE_ERR,"audiocoreaudio_end AudioUnitSetProperty failed");
		return;
	}

	/*if (CloseComponent(gOutputUnit) != noErr) {
		debug_printf (VERBOSE_ERR,"audiocoreaudio_end CloseComponent failed");
		return;
	}*/

}



int audiocoreaudio_fifo_write_position=0;
int audiocoreaudio_fifo_read_position=0;

//nuestra FIFO

//#define FIFO_BUFFER_SIZE (AUDIO_BUFFER_SIZE*10)
#define FIFO_BUFFER_SIZE (AUDIO_BUFFER_SIZE*4)


char audiocoreaudio_fifo_buffer[FIFO_BUFFER_SIZE];

//retorna numero de elementos en la fifo
int audiocoreaudio_fifo_return_size(void)
{
	//si write es mayor o igual (caso normal)
	if (audiocoreaudio_fifo_write_position>=audiocoreaudio_fifo_read_position) return audiocoreaudio_fifo_write_position-audiocoreaudio_fifo_read_position;

	else {
		//write es menor, cosa que quiere decir que hemos dado la vuelta
		return (FIFO_BUFFER_SIZE-audiocoreaudio_fifo_read_position)+audiocoreaudio_fifo_write_position;
	}
}

//retornar siguiente valor para indice. normalmente +1 a no ser que se de la vuelta
int audiocoreaudio_fifo_next_index(int v)
{
	v=v+1;
	if (v==FIFO_BUFFER_SIZE) v=0;

	return v;
}

//escribir datos en la fifo
void audiocoreaudio_fifo_write(char *origen,int longitud)
{
	for (;longitud>0;longitud--) {

		//ver si la escritura alcanza la lectura. en ese caso, error
		if (audiocoreaudio_fifo_next_index(audiocoreaudio_fifo_write_position)==audiocoreaudio_fifo_read_position) {
			debug_printf (VERBOSE_DEBUG,"audiocoreaudio FIFO full");
			return;
		}

		audiocoreaudio_fifo_buffer[audiocoreaudio_fifo_write_position]=*origen++;
		audiocoreaudio_fifo_write_position=audiocoreaudio_fifo_next_index(audiocoreaudio_fifo_write_position);
	}
}


//leer datos de la fifo
//void audiocoreaudio_fifo_read(char *destino,int longitud)
void audiocoreaudio_fifo_read(uint8_t *destino,int longitud)
{
	for (;longitud>0;longitud--) {



                if (audiocoreaudio_fifo_return_size()==0) {
                        debug_printf (VERBOSE_INFO,"audiocoreaudio FIFO empty");
                        return;
                }




                //ver si la lectura alcanza la escritura. en ese caso, error
                //if (audiocoreaudio_fifo_next_index(audiocoreaudio_fifo_read_position)==audiocoreaudio_fifo_write_position) {
                //        debug_printf (VERBOSE_DEBUG,"FIFO vacia");
                //        return;
                //}



                *destino++=audiocoreaudio_fifo_buffer[audiocoreaudio_fifo_read_position];
                audiocoreaudio_fifo_read_position=audiocoreaudio_fifo_next_index(audiocoreaudio_fifo_read_position);
        }
}


//puntero del buffer
//el tema es que nuestro buffer tiene un tamanyo y mac os x usa otro de diferente longitud
//si nos pide menos audio del que tenemos, a la siguiente vez se le enviara lo que quede del buffer


OSStatus coreaudiowrite( void *inRefCon GCC_UNUSED,
                         AudioUnitRenderActionFlags *ioActionFlags GCC_UNUSED,
                         const AudioTimeStamp *inTimeStamp GCC_UNUSED,
                         UInt32 inBusNumber GCC_UNUSED,
                         UInt32 inNumberFrames,
                         AudioBufferList *ioData )
{
  int len = deviceFormat.mBytesPerFrame * inNumberFrames;
  uint8_t* out = ioData->mBuffers[0].mData;



	//si esta el sonido desactivado, enviamos silencio
	if (audio_playing.v==0) {
		uint8_t *puntero_salida;
		puntero_salida = out;
		while (len>0) {
			*puntero_salida=0;
			puntero_salida++;
			len--;
		}
		//printf ("audio_playing.v=0 en audiocoreaudio\n");
	}

	else {

		//printf ("coreaudiowrite. longitud pedida: %d AUDIO_BUFFER_SIZE: %d\n",len,AUDIO_BUFFER_SIZE);
		if (len>audiocoreaudio_fifo_return_size()) {
			//debug_printf (VERBOSE_DEBUG,"FIFO is not big enough. Length asked: %d audiocoreaudio_fifo_return_size: %d",len,audiocoreaudio_fifo_return_size() );
			//esto puede pasar con el detector de silencio

			//retornar solo lo que tenemos
			//audiocoreaudio_fifo_read(out,audiocoreaudio_fifo_return_size() );

			return noErr;
		}


		else {
			audiocoreaudio_fifo_read(out,len);
		}

	}



return noErr;

}
