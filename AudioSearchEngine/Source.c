#define _CRT_SECURE_NO_DEPRECATE	
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#define MAX_CHANNEL 2
#define BUF_SIZE 1024

//prototype declarartions
int numItems(int frames, int channels);
void printInfo(int frames, int channels, int sampleRate);
void insertWatermark(double *buffer, int count, int channels);
void readFile(int fileNumber);
void printBuffer(double *buffer, size_t count);
void retrieveWatermark(double *buffer, int count, int channels, int chunkCount);


char *Files[] = { "D:/Jon/Downloads/File1.wav", "D:/Jon/Downloads/File2.wav", "D:/Jon/Downloads/File3.wav"
				, "D:/Jon/Downloads/File4.wav", "D:/Jon/Downloads/File5.wav" };

char *Files2[] = { "D:/Jon/Downloads/File1Output.wav", "D:/Jon/Downloads/File2Output.wav", "D:/Jon/Downloads/File3Output.wav"
				, "D:/Jon/Downloads/File4Output.wav", "D:/Jon/Downloads/File5Output.wav" };

int watermarkInterval = 4;

bool isStartOfSong = 0;



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

	printf("Would you like to read the new file?\n");
	printf("1. Yes\n");
	printf("2. No\n");
	scanf("%d", &answer);

	if (answer == 1)
	{	
		clock_t start = clock(), diff;
		readFile(chosenFile);
		diff = clock() - start;
		int msec = diff * 1000 / CLOCKS_PER_SEC;
		printf("CPU time taken to find watermarks %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
	}
	

	

}

int processFile(int fileNumber)
{
	/*READING FILE*/

	static double buffer[BUF_SIZE];
	SF_INFO info;
	SNDFILE *infile,*outfile;
	int readCount, i;

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
	printf("Number of Items(frames*channels): %d \n", num);
	

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
	it will keep reading/writing in 1024 chunks until all numItems have been read/written (numItems/BUF_SIZE)
	Needs to be on a while loop otherwise it will only write the first 1024 frames of the file
	*/

	int chunksPerSecond = (info.samplerate / info.channels) / BUF_SIZE;
	int chunksPerWatermark = chunksPerSecond * watermarkInterval;
	long chunkCount = 0;
	while ((readCount = sf_read_double(infile, buffer, BUF_SIZE)))
	{	
		/*increases total frames in the file instead of overwriting original value*/
		chunkCount++;
		if (chunkCount % chunksPerWatermark == 0)
		{
			printf("Watermark count: %d\n", chunkCount);
			insertWatermark(buffer, readCount, info.channels);
			sf_write_double(outfile, buffer, readCount);
		}
		sf_write_double(outfile, buffer, readCount);	
		
	};

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
	SNDFILE *infile2;
	int readCount;
	static double buffer2[BUF_SIZE];
	

	char *inputFile = Files2[fileNumber - 1];

	//infile = sf_open(inputFile, SFM_READ, &info);
	if (!(infile2 = sf_open(inputFile, SFM_READ, &info)))
	{
		printf("Not able to open input file %s.\n", inputFile);
		puts(sf_strerror(NULL));
		return 1;
	};

	printf("You have opened: %s\n", Files2[fileNumber - 1]);
	printInfo(info.frames, info.channels, info.samplerate);

	/*sets up to read where the watermark has been inserted*/
	int chunksPerSecond = (info.samplerate / info.channels) / BUF_SIZE;
	int chunksPerWatermark = chunksPerSecond * watermarkInterval;
	long chunkCount = 0;
	
	while ((readCount = sf_read_double(infile2, buffer2, BUF_SIZE)))
	{

		chunkCount++;
		if (chunkCount % chunksPerWatermark == 0)
		{
			printf("Watermark count: %d\n", chunkCount);
			//retrieveWatermark(buffer2, readCount, info.channels, chunkCount);
		}	
		retrieveWatermark(buffer2, readCount, info.channels, chunkCount);
	}
	
	sf_close(infile2);
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

double  watermark[] = { 0.0, 0.0, 9.0, 9.0, 1.0, 1.0, 9.0, 9.0, 1.0, 1.0, 9.0, 9.0,
		0.6, 0.6, 0.7, 0.7, 0.8, 0.8, 0.9, 0.9 };
double scaleFactor = 2; // to reduce the volume of the watermark
double baseline = 0.0567;
double temp = (1 / 100) * 2 + 0.0567;

void insertWatermark(double *buffer, int count, int channels)
{
	
	int i, j, k;

		/*
		Sets j to be the first channel and while i is less than 1024/5, i += channels
		buffer[3] value is multiplied by 0, and set to 0
		this mutes that particular index value or frame
		this keeps going until i>=1024/5 and then the next channel is chosen where j = 2
		buffer[4] value is multiplied by 0 and set to 0
		this keeps going until i>=1024/5 where it calls back to the while loop in processFile
		*/

	int duplicates = 2;
	for (i = 0; i < 20; i++)
		// insert duplicate of digit, in all channels
		for (j = 0; j < duplicates; j++)
			for (k = 0; k < channels; k++)
			{
				buffer[i*duplicates*channels + j*channels + k] = ((watermark[i]/100) * scaleFactor)	+ baseline;
				
			}
	
	return;
}

void retrieveWatermark(double *buffer, int count, int channels, int chunkCount)
{
	//buffer comes in 1024
	int i, j, k;
	int watermarkCounter = 0;
	bool startOfWatermark = 0;
	int idxOfWatermarkStart = 0;
	int numbersToCount = 4 * channels;

	/*
	Goes through the 1024 chunk and stops if a value is == 0
	Checks the next 3 values if they are all 0 and increments the counter each time
	Compares if there are 4 zeros for 1 channel, and 8 if 2 channels and
	Sets boolean to true and save the index position
	*/

	for (i = 0; i < count; i++)
	{
		if (!isStartOfSong)
		{
			if (buffer[i] != 0){
				isStartOfSong = 1;
			}
		}
		else 
			if (fabs( ((watermark[0] / 100)* scaleFactor + baseline) - buffer[i]) <= 0.000031)
				//if (buffer[i] == 0)
			{

				watermarkCounter = 0;

				for (j = 0; j < numbersToCount; j++)
				{
					int idxToCheck = i + j;

					if (fabs (((watermark[1] / 100)* scaleFactor + baseline) - buffer[idxToCheck]) <= 0.000031)
						//if (buffer[idxToCheck] == 0)
					{
						watermarkCounter++;
						if (watermarkCounter == numbersToCount)
						{
							startOfWatermark = 1;
							idxOfWatermarkStart = i;
						}
					}

				}

			}
	}
	/*
	Checks if boolean is true and prints the next 40 or 80 positions depending on the number of channels and
	their values
	*/
	if (startOfWatermark)
	{
		for (k = idxOfWatermarkStart; k < idxOfWatermarkStart + (20 * 2 * channels); k++)
		{
			printf("Watermark position in file is: %d\nValue: %lf\n", BUF_SIZE*chunkCount + k, fabs(buffer[k]));
		}
		
	}
	return;
}
	
/*
- *DONE* - Need to create function that will read the newly outputted file 
- *DONE* - Find where the watermarks have been left on the audio
- Compare buffer[] values between original file and new outputted file
*/