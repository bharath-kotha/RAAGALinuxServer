#include <stdio.h>
#include "alsa/asoundlib.h"
#include "alsa/control.h"

int main()
{
	// Handle for pcm device
	snd_pcm_t * pcm_handle;

	// Playback stream
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

	// Hardware information
	snd_pcm_hw_params_t * hwparams;

	// Software parameters
	snd_pcm_sw_params_t * swparams;

	// PCM information
	snd_pcm_info_t * info;

	// Name of the pcm device
	char * pcm_name;

	// Init pcm name
	pcm_name = strdup("default");

	// Allocate the memory on the stack
	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
	snd_pcm_info_alloca(&info);

	//Open PCM device
	int err = snd_pcm_open(&pcm_handle, pcm_name, stream,0);
	if(err < 0)
	{
		fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
		return (-1);
	}

	err = snd_pcm_info(pcm_handle, info);
	if(err <0)
	{
		fprintf(stderr, "Info error %s\n", snd_strerror(err));
		return (1);
	}

	// Init hwparams with full configuration space
	err =snd_pcm_hw_params_any(pcm_handle, hwparams) ;
	if(err < 0)
	{
		fprintf(stderr, "Can't configure this PCM device. \n");
		return (-1);
	}

	unsigned int rate = 48000;			// sample rate
	unsigned int exact_rate;				// sample rate returned
	int dir;
	int periods = 2;
	snd_pcm_uframes_t period_size = 	8192; 	//Period size(bytes)
	snd_pcm_uframes_t buffer_size = 8192 * 4;	// Buffer size

	// Set access type
	err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(err < 0)
	{
		fprintf(stderr, "Error setting access\n");
		return (-1);
	}

	// Set sample format
	err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S32_LE);
	if(err <0)
	{
		fprintf(stderr, "Error setting format\n");
		return (-1);
	}

	//Set number of channels
	err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2);
	if(err <0)
	{
		fprintf(stderr, "Error setting channels\n");
		return(-1);
	}

	// Set sample rate
	err = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, 0);
	if(err < 0)
	{
		fprintf(stderr, "Error setting sample rate\n");
		return(1);
	}

	//Set buffer size and period
	unsigned int buffer_time;
	unsigned int period_time;
	err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0);
	if(err < 0)
	{
		fprintf(stderr, "Error retrieving maximum buffer time");
		return (1);
	}
	if(buffer_time > 500000)
	{
		buffer_time = 500000;
	}
	period_time = buffer_time / 4;
	err = snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &period_time, 0);
	if(err < 0)
	{
		fprintf(stderr, "Error setting period time");
	}
	err = snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &buffer_time, 0);
	if(err < 0)
	{
		fprintf(stderr, "Error setting buffer time");
	}

	// Install hw params
	err = snd_pcm_hw_params(pcm_handle, hwparams);
	if(err <0)
	{
		fprintf(stderr, "Error setting HW parameters\n");
		return(-1);
	}

	// Get buffer and period size
	snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);

	//unsigned char * data;
	//int pcmreturn, l1, l2;
	//short s1,s2;
	//int frames;

	//data=(unsigned char *) malloc (period_size);
	//frames = period_size >> 3;


	// Set up Software parametes
	snd_pcm_sw_params_current(pcm_handle, swparams);

	// avail min
	err = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, period_size);
	if(err < 0)
	{
		fprintf(stderr, "Error setting available space");
	}

	// start threshold
	err = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, period_size);
	if(err < 0)
	{
		fprintf(stderr, "Error setting start threshold");
	}

	// Stop threshold
	err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, 1);
	if(err <0)
	{
		fprintf(stderr, "Error setting stop threshold");
	}

	// Install SW params
	err = snd_pcm_sw_params(pcm_handle, swparams);
	if(err < 0)
	{
		fprintf(stderr, "Error installing software parameters");
	}








	/*for(l1 = 0; l1 < 1000; l1++)
	{
		for(l2 = 0; l2 <frames; l2++)
		{
			s1 = (l2 % 128) * 100 - 5000;
			s2 = (l2 % 256) * 100 - 5000; 
			data [4 * l2] = (unsigned char) s1;
			data[4 * l2 +1] = s1 >> 8;
			data[4 * l2 + 2] = (unsigned char) s1;
			data[4 * l2 + 3] = s1 >> 8;
			data [4 * l2 + 4] = (unsigned char) s2;
			data[4 * l2 +5] = s2 >> 8;
			data[4 * l2 + 6] = (unsigned char) s2;
			data[4 * l2 + 7] = s2 >> 8;
		}
		while((pcmreturn = snd_pcm_writei(pcm_handle, data, frames)) < 0)
		{
			snd_pcm_prepare(pcm_handle);
			fprintf(stderr, "Buffer Underrun");
		}
	}*/

	snd_pcm_drain(pcm_handle);

	return 0;
}
