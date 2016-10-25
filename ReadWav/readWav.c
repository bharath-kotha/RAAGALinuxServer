#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Global variables


// Function declartions
void readWav(FILE * fhandle);
int main() {
// Open the file in read and binary mode
// fp holds the handle for file
FILE *fp;
fp = fopen("sample.WAV","rb");

readWav(fp);

fclose(fp);
return 0;
}

void readWav(FILE * fhandle)
{
	char c[4];

	// Check if the file header is intact
	fread(c,1,4,fhandle);
	if(!(c[0]=='R' && c[1] =='I' && c[2] == 'F' && c[3] == 'F'))
	{
		printf ("Something wrong with the file. Check the file format");
		return ;
	} 

	// Get the file size
	int32_t filesize;
	fread(&filesize,4,1,fhandle);
	//printf("%i",filesize);

	// Check for the WAVE header
	fread(c,1,4,fhandle);
	if(!(c[0]=='W' && c[1] =='A' && c[2] == 'V' && c[3] == 'E'))
	{
		printf ("Not a WAVE file. Check the file");
		return ;
	} 

	// Wave file contains fmt followed by PCM format
	// Check for fmt tag
	fread(c,1,4,fhandle);
	if(!(c[0]=='f' && c[1] =='m' && c[2] == 't' && c[3] == ' '))
	{
		printf ("File doesn't contain PCM format tag");

		return ;
	} 

	// Read format size
	int32_t fmtSize;
	fread(&fmtSize,4,1,fhandle);
	//printf("%i",fmtSize);

	// Read audio format
	int16_t audioFormat;
	fread(&audioFormat, 2, 1, fhandle);
	//printf("%i\n", audioFormat);

	// Read number of channels
	int16_t numChannels;
	fread(&numChannels,2,1,fhandle);
	//printf("%i\n", numChannels);

	// Read sample rate from the file
	int32_t sampleRate;
	fread(&sampleRate, 4, 1, fhandle);
	//printf("Sample Rate: %i\n", sampleRate);

	// Read byte rate / average bytes for second
	int32_t byteRate;
	fread(&byteRate, 4, 1, fhandle);
	//printf("Average Bytes per second: %i\n", byteRate);

	// Read block align / bytes per frame value
	int16_t blockAlign;
	fread(&blockAlign, 2, 1, fhandle);
	//printf("Block align: %i\n", blockAlign);

	// Read bits per sample
	int16_t bitsPerSample;
	fread(&bitsPerSample, 2, 1, fhandle);
	//printf("Bits per sample: %i\n", bitsPerSample);

	// Read size of extra bites
	int16_t cbSize;
	fread(&cbSize, 2, 1, fhandle);
	//printf("Extra Size: %i\n", cbSize);

	// Read and discard next cbSize of data
	if(cbSize != 0)
	{
		char * temp =(char *) malloc(cbSize*sizeof(char));
		if(temp == NULL)
		{
			printf("Error allocating memory");
			return;
		}
		fread(temp,sizeof(char), (int)cbSize, fhandle);
		free(temp);
		temp = NULL;
	}
	
	
	// Check for data tag
	fread(c,1,4,fhandle);
	if(!(c[0]=='d' && c[1] =='a' && c[2] == 't' && c[3] == 'a'))
	{
		printf ("Doesn't contain data tag\n");
		return ;
	} 

	// Read the data segment size
	int32_t dataSize;
	fread(&dataSize, 4, 1, fhandle);
	//printf("Data size: %i\n", dataSize);

	// Read the data
	char * buffer = (char *) malloc(dataSize * sizeof(char));
	fread(buffer, sizeof(char), dataSize, fhandle);
	int i;
	for(i=0; i< 20; i++)
	{
		printf("%c", buffer[i]);
	}
	free(buffer);

}
