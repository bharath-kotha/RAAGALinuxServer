/*********************
include files
*********************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <alsa/asoundlib.h>
#include <stdint.h>

/*********************
global variables
*********************/
int sock, cli;		// Client and server socket handles
struct sockaddr_in server, client;		// Server and client address
unsigned int len = sizeof(struct sockaddr_in);		// Size of sockaddr_in structure
snd_pcm_t * pcm_handle;				// Handle for PCM device
char * pcm_name;					// Name of the PCM device
snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;		// Direction of the playback stream
unsigned int sample_rate;			// Sample rate of the wave file
unsigned int num_channels;			// Number of channels in wave file
unsigned int buffer_time;			// Buffer length in micro seconds
unsigned int period_time;			// Period length in micro seconds
snd_pcm_uframes_t period_size_frames;	// Period size in frames
snd_pcm_uframes_t buffer_size_frames;	// Buffer size in frames
unsigned char * audio_buffer;				// Holds audio data after wave file reading
unsigned int frame_size;			// Size of each frame in bytes
unsigned int total_frames;			// Total number of frames in wave file



/*********************
function declarations
*********************/

// initializes socket
// Binds adress to socket
// sets max number of connections
void init_socket(void);

// A helper function to read the data from wave file
void read_wave_file(FILE * handle);

// A helper function to setup hw parameters for ALSA
void setup_hw_params(void);

// A helper function to setup sw parameters for ALSA
void setup_sw_params(void);

// A helper function to open PCM device
void open_pcm(void);

// Send wave file data to PCM device
void play_wave_file(void);

// A helper function for play_wave_file to write data to PCM devie
ssize_t  write_pcm(u_char * data, size_t count);



int main(int argc, char ** argv)
{
	
	pcm_name =strdup("default");
	char * file_name = argv[1];
	FILE * fHandle = fopen(file_name, "rb");
	read_wave_file(fHandle);

	open_pcm();
	setup_hw_params();
	setup_sw_params();

	//play_wave_file();

	fclose(fHandle);
	init_socket();	


	char recvBuffer[4800];
	while(1)
	{
		if((cli = accept(sock, (struct sockaddr *) & client, &len)) == -1)
		{
			fprintf(stderr, "accept");
			exit(-1);
		}

		while(true)
		{
			recv(cli, recvBuffer, sizeof(recvBuffer), 0);
			write_pcm((u_char *)recvBuffer, period_size_frames);
//			int i = 0;
//			for ( i = 0; i < 4800; i++)
//			{
//				printf("%c",recvBuffer[i]);
//			}
		}
	}

	


return 0;
}

void init_socket(void)
{

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr,"Error creating sockets");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(1111);
	server.sin_addr.s_addr = INADDR_ANY;
	//memset(&server.sin_zero,0,8 );
//	int i;
//	for(i =0 ; i< 8 ; i++)
//	{
//		server.sin_zero[i] = 0;
//	}
	bzero(&server.sin_zero, 8);

	len = sizeof(struct sockaddr_in);
	if(bind(sock,(struct sockaddr*) &server, len) == -1)
	{
		fprintf(stderr, "Bind\n");
		exit(1);
	}

	if(listen(sock,5) == -1)
	{
		fprintf(stderr, "listen");
		exit(1);
	}
}

void open_pcm(void)
{
	snd_pcm_info_t * info;
	snd_pcm_info_alloca(&info);
	int err = snd_pcm_open(&pcm_handle, pcm_name, stream,0);
	if(err < 0)
	{
		fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
		exit (-1);
	}

	err = snd_pcm_info(pcm_handle, info);
	if(err <0)
	{
		fprintf(stderr, "Info error %s\n", snd_strerror(err));
		exit (1);
	}
}


void read_wave_file(FILE * handle)
{
	char c[4];

	// Check if first 4 letters are RIFF
	fread(c,1,4,handle);
	if(!(c[0]=='R' && c[1] =='I' && c[2] == 'F' && c[3] == 'F'))
	{
		printf ("Something wrong with the file. Check the file format\n");
		return ;
	} 

	// Get the file size in bytes
	int32_t filesize;
	fread(&filesize,4,1,handle);
	#ifdef DEBUG
	printf("WAVE file size: %i\n",filesize);
	#endif

	// Check for the WAVE header
	fread(c,1,4,handle);
	if(!(c[0]=='W' && c[1] =='A' && c[2] == 'V' && c[3] == 'E'))
	{
		printf ("Not a WAVE file. Check the file\n");
		return ;
	} 

	// Wave file contains fmt followed by PCM format
	// Check for fmt tag
	fread(c,1,4,handle);
	if(!(c[0]=='f' && c[1] =='m' && c[2] == 't' && c[3] == ' '))
	{
		printf ("File doesn't contain PCM format tag\n");

		return ;
	} 

	// Read format size
	int32_t fmtSize;
	fread(&fmtSize,4,1,handle);
	#ifdef DEBUG
	printf("WAVE file format size: %i\n",fmtSize);
	#endif

	// Read audio format
	int16_t audioFormat;
	fread(&audioFormat, 2, 1, handle);
	#ifdef DEBUG
	printf("Audio Format: %i\n", audioFormat);
	#endif

	// Read number of channels
	int16_t numChannels;
	fread(&numChannels,2,1,handle);
	num_channels = numChannels;
	#ifdef DEBUG
	printf("Number of Channels: %i\n", numChannels);
	#endif

	// Read sample rate from the file
	uint32_t sampleRate;
	fread(&sampleRate, 4, 1, handle);
	sample_rate = sampleRate;
	#ifdef DEBUG
	printf("Sample Rate: %i\n", sampleRate);
	#endif

	// Read byte rate / average bytes for second
	int32_t byteRate;
	fread(&byteRate, 4, 1, handle);
	#ifdef DEBUG
	printf("Average Bytes per second: %i\n", byteRate);
	#endif

	// Read block align / bytes per frame value
	int16_t blockAlign;
	fread(&blockAlign, 2, 1, handle);
	#ifdef DEBUG
	printf("Block align: %i\n", blockAlign);
	#endif

	// Read bits per sample
	int16_t bitsPerSample;
	fread(&bitsPerSample, 2, 1, handle);
	#ifdef DEBUG
	printf("Bits per sample: %i\n", bitsPerSample);
	#endif

	// Read size of extra bites
	int16_t cbSize;
	fread(&cbSize, 2, 1, handle);
	#ifdef DEBUG
	printf("Extra Size: %i\n", cbSize);
	#endif

	// Read and discard next cbSize of data
	if(cbSize != 0)
	{
		char * temp =(char *) malloc(cbSize*sizeof(char));
		if(temp == NULL)
		{
			printf("Error allocating memory");
			return;
		}
		fread(temp,sizeof(char), (int)cbSize, handle);
		free(temp);
		temp = NULL;
	}
	
	
	// Check for data tag
	fread(c,1,4,handle);
	if(!(c[0]=='d' && c[1] =='a' && c[2] == 't' && c[3] == 'a'))
	{
		printf ("Doesn't contain data tag\n");
		return ;
	} 

	// Read the data segment size
	int32_t dataSize;
	fread(&dataSize, 4, 1, handle);
	#ifdef DEBUG
	printf("Data size: %i\n", dataSize);
	#endif

	// Read the data
	audio_buffer = (unsigned char *) malloc(dataSize * sizeof(unsigned char));
	if(audio_buffer == NULL)
	{
		printf("Failed to allocate memory");
	}
	fread(audio_buffer, sizeof(unsigned char), dataSize, handle);
	#ifdef DEBUG
	int i;
	for(i=0; i< 20; i++)
	{
		//printf("%c", audio_buffer[i]);
	}
	printf("\n");
	#endif

	frame_size = numChannels * bitsPerSample / 8;
	total_frames = dataSize / frame_size;
}


void setup_hw_params(void)
{
	snd_pcm_hw_params_t * hwparams;
	snd_pcm_hw_params_alloca(&hwparams);
	
	// Init hwparams with full configuration space
	int err =snd_pcm_hw_params_any(pcm_handle, hwparams) ;
	if(err < 0)
	{
		fprintf(stderr, "Can't configure this PCM device. \n");
		exit (-1);
	}

	unsigned int exact_rate;				// sample rate returned

	// Set access type
	err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(err < 0)
	{
		fprintf(stderr, "Error setting access\n");
		exit (-1);
	}

	// Set sample format
	err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_FLOAT_LE);
	if(err <0)
	{
		fprintf(stderr, "Error setting format\n");
		exit (-1);
	}

	//Set number of channels
	err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, num_channels);
	if(err <0)
	{
		fprintf(stderr, "Error setting channels\n");
		exit(-1);
	}

	// Set sample rate
	err = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &sample_rate, 0);
	if(err < 0)
	{
		fprintf(stderr, "Error setting sample rate\n");
		exit(1);
	}

	//Set buffer size and period
	err = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time, 0);
	if(err < 0)
	{
		fprintf(stderr, "Error retrieving maximum buffer time");
		exit (1);
	}
	if(buffer_time > 50000)
	{
		buffer_time = 50000;
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
	#ifdef DEBUG
	printf("Period_time: %i\n", period_time);
	printf("buffer_time: %i\n", buffer_time);
	#endif

	// Install hw params
	err = snd_pcm_hw_params(pcm_handle, hwparams);
	if(err <0)
	{
		fprintf(stderr, "Error setting HW parameters\n");
		exit(-1);
	}

	// Get buffer and period size
	snd_pcm_hw_params_get_period_size(hwparams, &period_size_frames, 0);
	snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size_frames);
	#ifdef DEBUG
	fprintf(stderr,"period_size: %i\n",(int) period_size_frames);
	fprintf(stderr,"buffer_size: %i\n", (int)buffer_size_frames);
	#endif

	//snd_output_t * log;
	//snd_output_stdio_attach(&log, stderr, 0);
	//snd_pcm_hw_params_dump(hwparams,log);
	//snd_pcm_dump(pcm_handle,log);
	//snd_output_close(log);
}

void setup_sw_params(void)
{
	snd_pcm_sw_params_t * swparams;
	snd_pcm_sw_params_alloca(&swparams);

	snd_pcm_sw_params_current(pcm_handle, swparams);

	// avail min
	int err = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, period_size_frames);
	if(err < 0)
	{
		fprintf(stderr, "Error setting available space");
		exit(1);
	}

	// start threshold
	err = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, buffer_size_frames);
	if(err < 0)
	{
		fprintf(stderr, "Error setting start threshold");
		exit(1);
	}

	// Stop threshold
	err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, swparams, buffer_size_frames);
	if(err <0)
	{
		fprintf(stderr, "Error setting stop threshold");
		exit(1);
	}

	#ifdef DEBUG
	fprintf(stderr, "avail_min: %i\n",(int) period_size_frames);
	fprintf(stderr, "start_threshold: %i\n",(int) buffer_size_frames);
	fprintf(stderr, "stop_threshold: %i\n",(int) buffer_size_frames);
	#endif

	// Install SW params
	err = snd_pcm_sw_params(pcm_handle, swparams);
	if(err < 0)
	{
		fprintf(stderr, "Error installing software parameters");
		exit(1);
	}
	snd_output_t * log;
	snd_output_stdio_attach(&log, stderr, 0);
	snd_pcm_sw_params_dump(swparams,log);
	snd_pcm_dump(pcm_handle,log);
	snd_output_close(log);
}

void play_wave_file(void)
{
	
	unsigned int written = 0;
	ssize_t result;

	//write_pcm(audio_buffer, period_size_frames * 4);
	
	while(written < total_frames)
	{
		//snd_pcm_wait(pcm_handle,100);
		result = write_pcm(audio_buffer + (written* frame_size), period_size_frames );
		//fprintf(stderr , "result : %i\n", result);
		written += result;
	}
	
	/*
	//printf("%c", audio_buffer[0]);
	//while((result = snd_pcm_writei(pcm_handle, audio_buffer, period_size_frames)) <0)
	{
		//snd_pcm_prepare(pcm_handle);
		//fprintf(stderr, "Buffer Underrun");
	}

	snd_pcm_drain(pcm_handle);
	*/
//	int l, r;
//	off64_t written = 0;
//	off64_t c;
//	size_t  loaded = total_frames;
//
//	fprintf(stderr, "Loaded: %i\n", loaded);
//	fprintf(stderr, "Count: %i\n", total_frames);
//	while ( loaded > period_size_frames && written < total_frames) {
//		if (write_pcm(audio_buffer + written, period_size_frames) <= 0)
//			return;
//		written += period_size_frames;
//		loaded -= period_size_frames;
//	}
	/*
	if (written > 0 && loaded > 0)
		memmove(audiobuf, audiobuf + written, loaded);

	l = loaded;
	while (written < count) {
		do {
			c = count - written;
			if (c > chunk_bytes)
				c = chunk_bytes;
			c -= l;

			if (c == 0)
				break;
			r = safe_read(fd, audiobuf + l, c);
			if (r < 0) {
				perror(name);
				prg_exit(EXIT_FAILURE);
			}
			fdcount += r;
			if (r == 0)
				break;
			l += r;
		} while ((size_t)l < chunk_bytes);
		l = l * 8 / bits_per_frame;
		r = pcm_write(audiobuf, l);
		if (r != l)
			break;
		r = r * bits_per_frame / 8;
		written += r;
		l = 0;
	}
	*/
	snd_pcm_nonblock(pcm_handle, 0);
	snd_pcm_drain(pcm_handle);
	snd_pcm_nonblock(pcm_handle,0 );
}



ssize_t  write_pcm(u_char * data, size_t count)
{
	ssize_t r;
	ssize_t result = 0;

	if (count < period_size_frames) {
		snd_pcm_format_set_silence(SND_PCM_FORMAT_S32_LE, data + count * frame_size, (period_size_frames - count) * num_channels);
		count = period_size_frames;
	}
	while (count > 0) {
		//snd_pcm_start(pcm_handle);
		r = snd_pcm_writei(pcm_handle, data, count);
		//fprintf(stderr, "Count: %i\n", count);
		int i;
//		for (i = 0; i< count ; i++)
//		{
//			printf("%c",data[i]);
//		}
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(pcm_handle, 100);
		} else if (r == -EPIPE) {
			//xrun();
			printf("xrun occured\n");
			snd_pcm_drain(pcm_handle);
			snd_pcm_prepare(pcm_handle);
			//snd_pcm_start(pcm_handle);
			//snd_pcm_reset(pcm_handle);
			//snd_pcm_drop(pcm_handle);
			//open_pcm();
			//setup_hw_params();
			//setup_sw_params();
			//snd_pcm_drop(pcm_handle);
		} else if (r == -ESTRPIPE) {
			//suspend();
		} else if (r < 0) {
			printf("Error writng data to PCM");
			exit(1);
		}
		if (r > 0) {
			result += r;
			count -= r;
			data += r * frame_size;
		}
	}
	return result;
}
