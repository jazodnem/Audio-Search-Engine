#define _CRT_SECURE_NO_DEPRECATE	
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

//sets the output file to be the same as the original file
void setInfo(SF_INFO *info1, SF_INFO *info2){
	info1->frames = info2->frames;
	info1->samplerate = info2->samplerate;
	info1->channels = info2->channels;
	info1->format = info2->format;
	info1->sections = info2->sections;
	info1->seekable = info2->seekable;
}

//prototype declarartions
void encodeLSB(int fileNum);
void decodeLSB(size_t frame, short *WatermarkFrames, size_t maxFrames, short *Frames, size_t byte, unsigned char *buffer)
;


char *Files[] = { "D:/Jon/Downloads/8bit Noise Mono.wav", "D:/Jon/Downloads/Star Wars Tune Mono.wav",
"D:/Jon/Downloads/Star Wars Tune Stereo.wav" };
char *EmbedFile = "D:/Jon/Documents/Final Year/CSC3002/Resources/embedFile.txt";
char *LSBFile = "D:/Jon/Downloads/Steg File.wav";

void encodeLSB(int fileNum)
{
	/*Need
	audio input, output, embed file(array)
	read input, match input with output
	*/

	SNDFILE *originalFile = NULL;
	SF_INFO originalInfo;
	SNDFILE *outputFile1 = NULL;
	SF_INFO outputInfo1;

	char *chosenFile = Files[fileNum - 1];

	//opens .wav file in READ mode to access RAW data
	originalFile = sf_open(chosenFile, SFM_READ, &originalInfo);
	//file check
	if (!originalFile){
		printf("sf_open error %s\n", sf_strerror(originalFile));
		return 1;
	}
	printf("You have opened the following file: %s\n", chosenFile);
	//printInfo(&originalInfo);

	//calculates max number of frames and allocates it accordingly
	size_t maxFrames = originalInfo.frames*originalInfo.channels;
	short *Frames = (short*)malloc(maxFrames*sizeof(short));

	FILE *embedfile = NULL;
	size_t bufferSize = 1000000;
	size_t size = 0;
	size_t byte = 0;
	unsigned char bit = 0;
	size_t frame = 0;
	int decode;

	//allocates buffer size to 1mil as unssigned char
	unsigned char *buffer = (unsigned char*)malloc(bufferSize);

	//stop reading original file
	sf_close(originalFile);

	//allocates the same number of frames to the output file as a the original file
	//sets outpput file info same as original in terms of frames/channels/sample rate/etc
	short *WatermarkFrames = (short*)malloc(maxFrames*sizeof(short));

	//setInfo(&outputInfo1, &originalInfo);

	//opens the output file in WRITE mode
	outputFile1 = sf_open(LSBFile, SFM_WRITE, &outputInfo1);
	if (!outputFile1){
		printf("sf_open error %s\n", sf_strerror(outputFile1));
		return 1;
	}

	//read and open .txt file
	embedfile = fopen(EmbedFile, "rb");
	if (!embedfile){
		printf("fopen error for %s\n", EmbedFile);
		return 1;
	}

	/*
	Encodes the data in the audio samples
	*/
	while (!feof(embedfile) && !ferror(embedfile)){
		//buffer = pointer to a block of memory with a minimum size of bbuffersize*1 byte
		//1 means reading elements 1 byte at a time
		//bufferSize = number of elements sized 1 byte
		//embedfile pointer to file to read
		//size changes depending on file read
		setInfo(&outputInfo1, &originalInfo);

		size = fread(buffer, 1, bufferSize, embedfile);


		for (byte = 0; byte < size; byte++){
			if (frame >= maxFrames - 9){
				printf("embed file will not fit!\n");
				return 1;
			}
			//goes through each bit for that particular byte
			for (bit = 0; bit < 8; bit++){

				//duplicates the key sample prior to lsb modification
				WatermarkFrames[frame] = Frames[frame];

				//Bitwise AND 1111 1111 1111 1110
				//set LSB of current frame to 0 and leave the rest untouched
				WatermarkFrames[frame] &= 0xFFFE;

				//sets the lsb of the audio sample to the lsb of one byte of the input data buffer
				WatermarkFrames[frame] |= (buffer[byte] & 0x01);

				//extract the bits being inserted
				int bitInserted = (buffer[byte] & 1);
				printf("Expected bit(s) = %u \n", bitInserted);

				//printf("Expected position = %u \n", WatermarkFrames + 8);


				//Assignment by bitwise right shift to iterate through the 8 component bits of the message byte
				buffer[byte] >>= 1;

				//next sample
				frame++;

			}
		}
	}

	/*
	for (int i = 0; i < 4; i++)
	{
	for (int j = 0; j < 25; j++)
	{
	WatermarkFrames[frame] = Frames[frame];

	WatermarkFrames[frame] &= 0xFFFE;

	if (Binary[j] == 0)
	{
	WatermarkFrames[frame] &= 0;
	}
	else
	WatermarkFrames[frame] |= 1;

	frame++;
	}
	}
	*/
	printf("Stored %ld bytes by altering %ld bits\n", frame / 8, frame);

	//duplicates the next audio sample after all data lsb samples have been encoded already
	WatermarkFrames[frame] = Frames[frame];

	//Bitwise OR 16 bit value;
	//flips the lsb of the next audio sample after the last lsb-modified data sample
	//makes it easier to decode later by using &=0x0001 operator
	WatermarkFrames[frame] ^= 0x0001;
	frame++;

	fclose(embedfile);

	//write the rest of the data without modifying them
	while (frame < maxFrames){
		WatermarkFrames[frame] = Frames[frame];
		frame++;
	}
	if (sf_write_short(outputFile1, WatermarkFrames, maxFrames) != maxFrames){
		printf("error writing steg file!\n");
		return 1;
	}
	sf_close(outputFile1);

	printf("Would you like to decode the file? 1. Yes 2. No\n");
	scanf("%d", &decode);

	if (decode == 1) {
		decodeLSB(frame, WatermarkFrames, maxFrames, Frames, byte, buffer);
	}

}


void decodeLSB(size_t frame, short *WatermarkFrames, size_t maxFrames, short *Frames, size_t byte, unsigned char *buffer)
{
	SNDFILE *outputFile = NULL;
	SF_INFO outputInfo;
	FILE *embedfile = NULL;

	//open steg file in read mode
	outputFile = sf_open(LSBFile, SFM_READ, &outputInfo);

	//open the embed file
	embedfile = fopen(EmbedFile, "wb");

	sf_close(outputFile);

	/*
	goes through all the frames of the output file and compares it to the original frames
	for any frames that does not match, it's counted as a bit inserted and later the total
	is divided by 8 to give the byte size altered.
	*/
	for (frame = maxFrames - 1; frame > 0; frame--){
		if (WatermarkFrames[frame] != Frames[frame]){
			printf("Number of bits inserted %ld representing %ld bytes\n", frame, frame / 8);
			maxFrames = frame;
			break;
		}
	}
	if (frame <= 0){
		printf("no embedded content detected!\n");
		return 1;

	}

	buffer = (unsigned char*)realloc(buffer, maxFrames / 8);
	if (!buffer){
		printf("buffer realloc error!\n");
		return 1;
	}
	//gets the total number of bytes in the file
	//Zeros out all the bits in the particular byte
	for (byte = 0; byte < maxFrames / 8; byte++){
		buffer[byte] = 0;
	}

	//
	for (frame = 0; frame < maxFrames; frame++){

		/*
		Bitwise AND the the current frame for all the frames of output file
		to show all the altered bits
		*/

		WatermarkFrames[frame] &= 0x0001;
		printf("Bits recovered %u \n", WatermarkFrames[frame] & 1);

		//buffer[frame / 8] |= (WatermarkFrames[frame] << frame % 8);
	}

}