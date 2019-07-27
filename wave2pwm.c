#include <stdio.h>
#include <stdlib.h>
#include "wiringPi.h"

#define PWM_PIN 26
typedef char int16;
#define RANGE 255
typedef struct WavHeader_
{
	// Header information.
	char		riff[4];		// RIFF
	int			chunkSize;		// Should be equal to: 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
	char		format[4];		// WAVE

	// Sub Chunk 1 information.
	char		subChunk1Id[4]; // Contains "fmt "
	int			subChunk1Size;	// 16 for PCM.
	short int	audioFormat; 	// PCM = 1
	short int	channels;		// Mono = 1, Stereo = 2
	int			sampleRate;  	// 8000, 16000, 44100, etc
	int			byteRate;		// sampleRate * channels * bitsPerSample / 8
	short int	blockAlign;		// channels * bitsPerSample / 8
	short int	bitsPerSample;  // 8 bits = 8, 16 bits = 16

	// Sub chunk 2 information.
	char		subChunk2Id[4];	// Contains "data"
	int			subChunk2Size;
}WavHeader;

void WavPrintHeader(WavHeader *pHeader);
void WavPlay(WavHeader *pHeader, int16 *aData, int iDataLength,int divisor);

void WavPrintHeader(WavHeader *pHeader)
{
	printf("\nWAV header Information:\n");
	printf("RIFF            : %c%c%c%c\n", pHeader->riff[0], pHeader->riff[1], pHeader->riff[2], pHeader->riff[3]);
	printf("Chunk Size      : %d\n", pHeader->chunkSize);
	printf("Format          : %c%c%c%c\n", pHeader->format[0], pHeader->format[1], pHeader->format[2], pHeader->format[3]);

	printf("\nSub-Chunk 1 Information:\n");
	printf("Sub-Chunk 1 ID  : %c%c%c%c\n", pHeader->subChunk1Id[0], pHeader->subChunk1Id[1], pHeader->subChunk1Id[2], pHeader->subChunk1Id[3]);
	printf("Sub-Chunk Size  : %d\n", pHeader->subChunk1Size);
	printf("Audio Format    : %d (PCM = 1)\n", pHeader->audioFormat);
	printf("Channels        : %d (MONO = 1, STERIO = 2)\n", pHeader->channels);
	printf("Sampling Rate   : %d Hz\n", pHeader->sampleRate);
	printf("Byte Rate       : %d (%d)\n", pHeader->byteRate, (pHeader->sampleRate * pHeader->channels * pHeader->bitsPerSample) / 8);
	printf("block Alignment : %d (%d)\n", pHeader->blockAlign, (pHeader->channels * pHeader->bitsPerSample) / 8);
	printf("Bits Per Sample : %d bits\n", pHeader->bitsPerSample);

	printf("\nSub-Chunk 2 Information:\n");
	printf("Sub-Chunk 2 ID  : %c%c%c%c\n", pHeader->subChunk2Id[0], pHeader->subChunk2Id[1], pHeader->subChunk2Id[2], pHeader->subChunk2Id[3]);
	printf("Sub-Chunk Size  : %d\n", pHeader->subChunk2Size);

	printf("\nGeneral Information:\n");
	printf("Audio Length    : %d seconds\n\n", pHeader->subChunk2Size / pHeader->byteRate);
}

void WavPlay(WavHeader *pHeader, int16 *aData, int iDataLength,int divisor)
{
	int16 *pDataEnd = aData + iDataLength;
    
	// Initialize the library.
	if (wiringPiSetup() != 0)
	{
	    printf("Error initializing wiringPi library.");
	    return;
	}

	// Set the PWM in information.
	pinMode(PWM_PIN, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_BAL);

	// Set the rang to 256 since we only accept 8-bit PCM files.
	pwmSetRange(255);

	// Set the clock pin on PWM. It has to be at lest the sample rate, but we multiply by 10 for better quality.
	//gpioClockSet (PWM_PIN, pHeader->sampleRate * 10);

	// This should be the same as the above but it isn't and I'm not sure way!!!!!
	//pwmSetClock( 19200000 / pHeader->sampleRate *  divisor);
	pwmSetClock(2);
	// We only support 8 bit per sample, so check for it.
        //if (pHeader->bitsPerSample != 8) { printf("\nSorry, unsupported bits per sample value, only accept 8bits/sample.\n"); return; }

	// Loop through the data and modulate accordingly.
	while (aData < pDataEnd)
	{
		//int16 *p = NULL;
		//p = aData;
		pwmWrite(PWM_PIN, *(aData++));
		aData++;
		//printf("%d\n",*p);
		delayMicroseconds( 1000000 / pHeader->sampleRate);
	}

	// Turn down the pin when done.
	pwmWrite(PWM_PIN, 0);
	pinMode(PWM_PIN, INPUT);
}


int main(int iArgumentCount,char *aArgumens[])
{
	FILE *pWavFile;
	WavHeader oWavHeader;

	int16 *aData;		// Holds the PCM WAV data read from the file.
	int iDataLength;
    int divisor = 10;
	//Ensure we have enough arguments to start.
	if(iArgumentCount < 2) { printf("\nYou forgot a wave input file name...\n"); exit(1); }

	// Open file wave file, read the header first then data later.
	if( (pWavFile = fopen(aArgumens[1], "r")) == 0) { printf("Ops, unable to open the given wave file!\n"); exit(2); }

	// Read the header from the file.
	fread(&oWavHeader, sizeof(WavHeader), 1, pWavFile);
	WavPrintHeader(&oWavHeader);

	printf("begin to read data from file;\n");

	// Read the data.
	aData = malloc(oWavHeader.subChunk2Size);
	iDataLength = (int)fread(aData, 1, oWavHeader.subChunk2Size, pWavFile);

	printf("\nRead data length from file: %d (%d)\n", iDataLength, oWavHeader.subChunk2Size);
    
	if ( iArgumentCount > 2 )
	{
	     divisor = atoi(aArgumens[2]);
	     printf("begin to play the file.\n");
	     WavPlay(&oWavHeader, aData, iDataLength,divisor);
	     printf("end to play the file.\n");
	}
	// Clean up and return.
	fclose(pWavFile);
	free(aData);

	return 0;
}
