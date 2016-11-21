#define _CRT_SECURE_NO_DEPRECATE	
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <time.h>

#define MAX_CHANNEL 2
#define BUF_SIZE 1024

int numItems(int frames, int channels);
void printInfo(int frames, int channels, int sampleRate);
void watermark(double *buffer, int count, int channels);
void watermark2(double *buffer, int count);
void readFile(int fileNumber);
void printBuffer(double *buffer, size_t count);

int main(void)
{

	int chosenFile, answer;
	
	printf("Please select the file you want to read\n");
	printf("1. File1\n");
	printf("2. File2\n");
	printf("3. File3\n");
	printf("4. File4\n");
	printf("5. File5\n");
	scanf("%d", &chosenFile);	
	
	processFile(chosenFile);
	/*
	Put boolean here to check whether to process the original file or the new one
	*/
	printf("Would you like to read the new file?\n");
	printf("1. Yes\n");
	printf("2. No\n");
	scanf("%d", &answer);

	if (answer == 1)
	{
		readFile(chosenFile);
	}

}

int processFile(int fileNumber)
{
	/*READING FILE*/

	static double buffer[BUF_SIZE];
	SF_INFO info;
	SNDFILE *infile,*outfile;
	int readCount, i;


	/*
	Put boolean here to check whether it should read the original files, or the new output files

	*/
	char *Files[] = { "D:/Jon/Downloads/File1.wav", "D:/Jon/Downloads/File2.wav", "D:/Jon/Downloads/File3.wav"
		, "D:/Jon/Downloads/File4.wav", "D:/Jon/Downloads/File5.wav" };

	char *Files2[] = { "D:/Jon/Downloads/File1Output.wav", "D:/Jon/Downloads/File2Output.wav", "D:/Jon/Downloads/File3Output.wav"
		, "D:/Jon/Downloads/File4Output.wav", "D:/Jon/Downloads/File5Output.wav" };

	char *inputFile = Files[fileNumber - 1];

	if (!(infile = sf_open(inputFile, SFM_READ, &info)))
	{
		printf("Not able to open input file %s.\n", inputFile);
		puts(sf_strerror(NULL));
		return 1;
	};

	
	printf("You have opened: %s\n", Files[fileNumber - 1]);
	printInfo( info.frames, info.channels, info.samplerate);
	int num = numItems(info.frames, info.channels);
	printf("Buffer(frames*channels): %d \n", num);
	

	/*WRITING FILE*/	
	char *outputFile = Files2[fileNumber - 1];
	printf("Your file has been saved in the following location: %s\n", outputFile);
	

	if (!(outfile = sf_open(outputFile, SFM_WRITE, &info)))
	{
		printf("Not able to open output file %s.\n", outputFile);
		puts(sf_strerror(NULL));
		return 1;
	};

	/*
	Actual buffer size is numItems, somehow can't declare buffer as buffer[numItems]
	BUF_SIZE is set to 1024, which means that it reads the data in chunks of 1024 items
	it will keep writing in 1024 chunks until all numItems have been written (numItems/BUF_SIZE)
	Needs to be on a while loop otherwise it will only write the first 1024 frames of the file
	*/

	
	while ((readCount = sf_read_double(infile, buffer, BUF_SIZE)))
	{	

		watermark(buffer, readCount, info.channels);
		sf_write_double(outfile, buffer, readCount);	
		printBuffer(buffer, sizeof(buffer) / sizeof *buffer);
		

	};
	/*
	When called after being written, it only reads the last 1024 items of the file which is the quietest 
	part and are all zeros.
	*/


	


	/*
	Can only close SF_open once both reading/writing has been done
	if you close infile after the read, it's not able to copy the audio
	data from infile to write into outfile
	*/
	sf_close(infile);
	sf_close(outfile);


	return;
}

void readFile(int fileNumber)
{
	SF_INFO info;
	SNDFILE *infile;
	int readCount;
	static double buffer[BUF_SIZE];

	char *Files[] = { "D:/Jon/Downloads/File1Output.wav", "D:/Jon/Downloads/File2Output.wav", "D:/Jon/Downloads/File3Output.wav"
		, "D:/Jon/Downloads/File4Output.wav", "D:/Jon/Downloads/File5Output.wav" };

	char *inputFile = Files[fileNumber - 1];

	infile = sf_open(inputFile, SFM_READ, &info);

	printf("You have opened: %s\n", Files[fileNumber - 1]);
	printInfo(info.frames, info.channels, info.samplerate);

	while ((readCount = sf_read_double(infile, buffer, BUF_SIZE)))
	{
		printBuffer(buffer, sizeof(buffer) / sizeof *buffer);
	};

	sf_close(infile);

	return;


}

int numItems(int frames, int channels)
{
	int numItems = frames * channels;
	return numItems;
}
void printInfo(int frames, int channels, int sampleRate)
{
	printf("Number of Frames = %d\n", frames);
	printf("Number of Channels = %d\n", channels);
	printf("Sample Rate = %d\n", sampleRate);
}

void watermark(double *buffer, int count, int channels)
{		
	double value[MAX_CHANNEL] = { 0.0, 0.0 };
	int i, j;

	if (channels > 1)
	{
		/*
		Sets j to be the first channel and while i is less than 1024/5, i += channels
		buffer[3] value is multiplied by 0, and set to 0
		this mutes that particular index value or frame
		this keeps going until i>=1024/5 and then the next channel is chosen where j = 2
		buffer[4] value is multiplied by 0 and set to 0
		this keeps going until i>=1024/5 where it calls back to the while loop in processFile
		*/

		for (j = 0; j < channels; j++)
		{
			for (i = j; i < count / 5; i += channels)
			{
				buffer[i] *= value[j];
			}
		}
	}
	else
	{
		/*
		If audio file has 1 channel, buffer[i] is set to 0 for all values where i < 1024/5 frames
		and it goes back to normal until the next 1024 frames where the first  1024/5 frames.
		*/
		for (i = 0; i < count / 5; i++)
		{
			buffer[i] *= value[0];
		}
	}
	
	return;
}

void printBuffer(double *buffer, size_t count)
{
	int i;	
	for (i = 0; i < count; i++)
	{
	/*
	Print the index position and value where value is not = 0
	*/

		if (buffer[i] != 0 && i < count)
			printf("Position: %d \nValue: %lf\n", i, buffer[i]);
		
		
	}
	
}

/*
- *DONE* - Need to create function that will read the newly outputted file 
- *DONE* - Find where the watermarks have been left on the audio
- Compare buffer[] values between original file and new outputted file
*/