#define _CRT_SECURE_NO_DEPRECATE	
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <stdbool.h>
#include <math.h>
#define MAX_CHANNEL 2
#define BUF_SIZE 1024
#define ROWS 10
#define COLUMNS 85


//sets the output file to be the same as the original file
void setInfo(SF_INFO *info1, SF_INFO *info2){
	info1->frames = info2->frames;
	info1->samplerate = info2->samplerate;
	info1->channels = info2->channels;
	info1->format = info2->format;
	info1->sections = info2->sections;
	info1->seekable = info2->seekable;
}

////prototype declarartions
int numItems(int frames, int channels);
void printInfo(int frames, int channels, int sampleRate);
void insertWatermark(double *buffer, int count, int channels);
int decodeInsertion(int fileNumber);
void printBuffer(double *buffer, size_t count);
void retrieveWatermark(double *buffer, int count, int channels, int chunkCount);
void getAccuracy(int *a, int *b);
void moveToArray();
void generateBits();
void increaseWM11secs();
void insertTrigger(int array[][COLUMNS]);
int encodeLSB(int fileNum, int bits);
int decodeLSB(int file);
void splitArray();
void binaryToDecimal(int array[][COLUMNS]);
int countFalsePos();
void insertionAccuracy(double a[], double b[]);

//array of original files
char *Files[] = { 
"Male Voice Recording.wav",
"Female Voice Recording.wav"
};

//array of output files for insertion watermarking
char *Files2[] = { "Male Voice Recording Insertion Output.wav", 
"Female Voice Recording Insertion Output.wav",
"C320 Male Voice Recording Insertion Output.wav",
"C320 Female Voice Recording Insertion Output.wav"
};

//output file of LSB encoding
char *LSBFile = "LSB encoded File.wav";

/*
C denotes compressed and number that follows is the bitrate used.
array of LSB watermarked files uncompressed
*/
char *WatermarkedFiles[] = {
"Uncompressed Male Voice Recording LSB.wav",
"Uncompressed Male Voice Recording 4LSB.wav",
"C128 Male Voice Recording LSB.wav",
"C128 Male Voice Recording 4LSB.wav",
"C256 Male Voice Recording LSB.wav",
"C256 Male Voice Recording 4LSB.wav",
"C320 Male Voice Recording LSB.wav",
"C320 Male Voice Recording 4LSB.wav",
"Uncompressed Female Voice Recording LSB.wav",
"Uncompressed Female Voice Recording 4LSB.wav",
"C128 Female Voice Recording LSB.wav",
"C128 Female Voice Recording 4LSB.wav",
"C256 Female Voice Recording LSB.wav",
"C256 Female Voice Recording 4LSB.wav",
"C320 Female Voice Recording LSB.wav",
"C320 Female Voice Recording 4LSB.wav"
};

char *EmbedFile = "Bits encoded.txt";
char *EmbedFile2 = "Bits decoded.txt";

int *Binary[10][4] = {
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 1 },
	{ 0, 0, 1, 0 },
	{ 0, 0, 1, 1 },
	{ 0, 1, 0, 0 },
	{ 0, 1, 0, 1 },
	{ 0, 1, 1, 0 },
	{ 0, 1, 1, 1 },
	{ 1, 0, 0, 0 },
	{ 1, 0, 0, 1 }
};

//make sure to double check the date for watermarks on audio files
//date and time formate of ss/mm/hh dd/mm/yy
int watermarkValues[12] = {0, 1, 3, 6, 1, 3, 1, 1, 0, 4, 1, 7};

//trigger bits used to mark the start of a watermark, needed to make it as uncommon of a sequence/pattern as possible.
int triggerBits[37] = { 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1 };

int watermarkSequence[ROWS][COLUMNS];
int decodedSequence[ROWS][COLUMNS];

char bitsEncoded[850];
char bitsDecoded[850];

//used for insertion method
int watermarkInterval = 4;
bool isStartOfSong = 0;
double insertionBitsEncoded[1360];
double insertionBitsDecoded[1360];


void generateBits() //function that generates 10 sequence of watermark values
{
	FILE *fp;
	fp = fopen(EmbedFile, "w");

	insertTrigger(watermarkSequence);
	printf("Watermark values used are displayed below in the following format\nss:mm:hh:dd:mm:yy\n");
	for (int wm = 0; wm < 10; wm++){
		// Write out watermarkValues[0...11]
		for (int value = 0; value < 12; value++){
			printf("%d", watermarkValues[value]);
		}
		printf("\n");
		
		//convert the values into binary representation
		for (int i = 0; i < 12; i++){
			for (int j = 0; j < 4; j++){
				watermarkSequence[wm][37 + (4 * i) + j] = Binary[watermarkValues[i]][j];
			}
		}

		//write conversion to text file including triggers
		for (int k = 0; k < 85; k++){
			fprintf(fp, "%d", watermarkSequence[wm][k]);
		}
		//increase the watermark value by 11secs
		increaseWM11secs();
	}
	fclose(fp);
}

//inserts the trigger bits into the start watermark 
void insertTrigger(int array[][COLUMNS])
{
	//picks the first sequence using row number and inserts 37 values into the columns
	for (int i = 0; i < 10; i++){
		for (int j = 0; j < 37; j++)
		{
			array[i][j] = triggerBits[j];
		}
	}
}
void increaseWM11secs()
{
	//increase watermark by 11secs
	watermarkValues[1] += 1;

	//checks so that there is no overflow in values like 69seconds
	if (watermarkValues[1] > 9){
		watermarkValues[1] = 0;
		watermarkValues[0] += 1;
	}
	watermarkValues[0] += 1;
	if (watermarkValues[0] >= 6){

		watermarkValues[0] = 0;
		watermarkValues[3] += 1;

		if (watermarkValues[3] > 9){
			watermarkValues[3] = 0;
			watermarkValues[2] += 1;
		}
	}
}

int main(void)
{
	/*
	file is used for file to encode
	method is used for WM method
	option is used for encode/decode
	bits is used for number of LSBs
	file2 is the file to decode
	*/
	int file, file2,file3, method,method2, option, bits;

	option = 1;
	while ((option == 1) || (option == 2))
	{

		printf("Do you want to\n1. Encode\n2. Decode\n3. Exit\n\n");
		scanf("%d", &option);

		if (option == 1)
		{
			printf("Please select the file you want to encode\n1. Male Voice Recording\n2. Female Voice Recording\n\n");
			scanf("%d", &file);

			printf("What method would you like to use?\n1. Insertion encoding\n2. LSB encoding\n\n");
			scanf("%d", &method);

			if (method == 1)
			{
				encodeInsertion(file);
			}
			else
			{
				//generate the bits that will be used for watermarking
				generateBits();

				printf("How many LSB would you like to encode?\n1. 1\n2. 4\n\n");
				scanf("%d", &bits);

				if (bits == 1)
				{
					encodeLSB(file, 1);
				}
				else
					encodeLSB(file, 2);
			}

		}
		else
		if (option == 2)
		{
			printf("What method would you like to use?\n1. Insertion decoding\n2. LSB decoding\n");
			scanf("%d", &method2);
			if (method2 == 1){
				printf("Please select the file you want to decode.\n");
				printf("1. Uncompressed Male Voice Recording\n2. Uncompressed Female Voice Recording\n");
				printf("3. Compressed to 320kbps Male Voice Recording\n4. Compressed to 320kbps Female Voice Recording\n");
				scanf("%d", &file3);
				decodeInsertion(file3);
				insertionAccuracy(insertionBitsEncoded,insertionBitsDecoded);
			}
			else
			{
				printf("Please select the file you want to decode.\n\n");
				printf("Male voice\n");
				printf("1. Uncompressed using LSB\n2. Uncompressed using 4LSB\n3. Compressed to 128kbps using LSB\n4. Compressed to 128kbps using 4LSB\n");
				printf("5. Compressed to 256kbps using LSB\n6. Compressed to 256kbps using 4LSB\n7. Compressed to 320kbps using LSB\n8. Compressed to 320kbps using 4LSB\n\n");
				printf("Female voice\n");
				printf("9. Uncompressed using LSB\n10. Uncompressed using 4LSB\n11. Compressed to 128kbps using LSB\n12. Compressed to 128kbps using 4LSB\n");
				printf("13. Compressed to 256kbps using LSB\n14. Compressed to 256kbps using 4LSB\n15. Compressed to 320kbps using LSB\n16. Compressed to 320kbps using 4LSB\n");
				scanf("%d", &file2);

				decodeLSB(file2); //passes the file number to decode function
				moveToArray(); //moves decoded sequence to an array
				splitArray(); // splits array into 10 rows by 85 columns
				binaryToDecimal(decodedSequence); //converts binary into decimal values
				printf("Number of false positives decoded: %d\n", countFalsePos()); //returns the number of false positives counted
				getAccuracy(bitsEncoded, bitsDecoded); //compares the decoded bits to encoded to get accuracy 
			}
		}

	}

}
int encodeLSB(int fileNum, int bits)
{
	SNDFILE *originalFile = NULL;
	SF_INFO originalInfo;
	SNDFILE *outputFile = NULL;
	SF_INFO outputInfo;

	char *chosenFile = Files[fileNum - 1];

	//opens the file in READ mode
	originalFile = sf_open(chosenFile, SFM_READ, &originalInfo);
	if (!originalFile){
		printf("sf_open error %s\n", sf_strerror(originalFile));
		return 1;
	}

	size_t frame = 0;
	int wmcounter = 0;
	size_t maxFrames = originalInfo.frames*originalInfo.channels;

	//allocates memory to hold each frame
	short *inputFrames = (short*)malloc(maxFrames*sizeof(short));
	if (!inputFrames){
		printf("Input frames malloc error!\n");
		return 1;
	}
	//read the data chunk in terms of items and saves it to inputFrames, also checks that maxFrames are the same.
	if (sf_read_short(originalFile, inputFrames, maxFrames) != maxFrames){
		printf("Corrupt input file!\n");
		return 1;
	}
	sf_close(originalFile);

	//allocates memory to hold each frame for the output file
	short *outputFrames = (short*)malloc(maxFrames*sizeof(short));
	if (!outputFrames){
		printf("Output frames malloc error!\n");
		return 1;
	}

	//sets outpput file info same as original in terms of frames/channels/sample rate/etc
	setInfo(&outputInfo, &originalInfo);

	//opens the output file in WRITE mode
	outputFile = sf_open(LSBFile, SFM_WRITE, &outputInfo);
	if (!outputFile){
		printf("sf_open error %s\n", sf_strerror(outputFile));
		return 1;
	}

	//encode watermarks into output samples
	while (frame < maxFrames){
		outputFrames[frame] = inputFrames[frame];
		frame++;

		if (bits == 1){
			//uses a different mask to only encode the LSB of the audio sample
			if (frame % 44100 == 0 && wmcounter < 10){
				//case 1000:
				for (int i = 0; i < 85; i++)
				{
					outputFrames[frame] = inputFrames[frame];
					//encode 0
					if (watermarkSequence[wmcounter][i] == 0){
						outputFrames[frame] &= ~1;
					}
					else
						//encode 1
						outputFrames[frame] |= 1;
					frame++;
				}
				wmcounter++;				
			}
		}
		//uses a different mask to encode the 4LSB instead
		else
			//uses a different mask to only encode the LSB of the audio sample
			if (frame % 44100 == 0 && wmcounter < 10){
				//case 1000:
				for (int i = 0; i < 85; i++)
				{
					outputFrames[frame] = inputFrames[frame];
					//encode 0
					if (watermarkSequence[wmcounter][i] == 0){
						outputFrames[frame] &= ~1;
					}
					else
						//encode 1
						outputFrames[frame] |= 1;
					frame++;
				}
				wmcounter++;				
			}

	}

	//writes data chunks in terms of items using outputFrames to the outputFile and checks that the numebr of frames is still the same.
	if (sf_write_short(outputFile, outputFrames, maxFrames) != maxFrames){
		printf("Error writing output file!\n");
		return 1;
	}

	printf("The file has been encoded succesfully.\n");
	printf("You can find the watermarked file at the following location:\n%s\n", outputFile);
	printf("The encoded bits can be found in:\n %s\n\n", EmbedFile);
	sf_close(outputFile);
}

int decodeLSB(int file)
{	
	SNDFILE *watermarkedFile = NULL;
	SF_INFO watermarkedInfo;
	FILE *fp, *ch;
	fp = fopen(EmbedFile2, "w");
	//unsigned int bits;
	
	//sets file chosen to be decoded
	char *wmFile = WatermarkedFiles[file - 1];

	//opens the output file in READ mode
	watermarkedFile = sf_open(wmFile, SFM_READ, &watermarkedInfo);
	if (!watermarkedFile){
		printf("sf_open error %s\n", sf_strerror(watermarkedFile));
		return 1;
	}

	size_t maxFrames2 = watermarkedInfo.frames*watermarkedInfo.channels;

	//malloc max frame for storing each frame into pointer
	short *outputFrames = (short*)malloc(maxFrames2*sizeof(short));
	if (!outputFrames){
		printf("Output frames malloc error!\n");
		return 1;
	}

	//reads data chunks in terms of items using outputFrames to the outputFile and checks that the numebr of frames is still the same.
	if (sf_read_short(watermarkedFile, outputFrames, maxFrames2) != maxFrames2){
		printf("Error writing output file!\n");
		return 1;
	}
	sf_close(watermarkedFile);

	int counter = 0;
	bool startOfWatermark = 0;
	int idxOfWatermarkStart = 0;
	int numOfBitsToCheck;

	//adjusts the counter to compensate for the compression of the files.
	if (file == 1 || file == 2 || file == 9 || file == 10){
		numOfBitsToCheck = 37;
	}
	else
		numOfBitsToCheck = 31;

	for (int frames = 0; frames < maxFrames2; frames++)
	{
		if ((outputFrames[frames] & 1) == 1){
			counter = 0;

			//compares 37 bits to the trigger sequence
			for (int i = 0; i < 37; i++){
				int idxToCheck = i + frames;

				if ((outputFrames[idxToCheck] & 1) == triggerBits[i]){
					counter++;

					//depending on sensitivity, decide whether it is a watermark sequence or not
					if (counter >= numOfBitsToCheck){
						startOfWatermark = 1;
						idxOfWatermarkStart = frames;
					}
				}
			}
		}
		//prints out the 85 bits if it is a watermark starting from the very first trigger
		if (startOfWatermark){
			for (int j = idxOfWatermarkStart; j < idxOfWatermarkStart + 85; j++){
				fprintf(fp, "%d", outputFrames[j] & 1);
				/*
				used to extract the 4LSBs of decoded sample
				bits = outputFrames[j] & 0xF;
				printf("%u\n", bits);
				*/
			}
			//adjust frame counter to read after the 85bits
			frames = idxOfWatermarkStart + 85;
			//reset startOfWatermark
			startOfWatermark = 0;
		}
	}
	fclose(fp);
	printf("The file has been decoded succesfully. \n");
	printf("The decoded bits can be found in:\n %s\n", EmbedFile2);

}
int encodeInsertion(int fileNumber)
{
	/*READING FILE*/

	static double buffer[BUF_SIZE];
	SF_INFO info;
	SNDFILE *infile, *outfile;
	int readCount;

	char *inputFile = Files[fileNumber - 1];

	if (!(infile = sf_open(inputFile, SFM_READ, &info)))
	{
		printf("Not able to open input file %s.\n", inputFile);
		puts(sf_strerror(NULL));
		return 1;
	};


	printf("You have opened: %s\n", Files[fileNumber - 1]);
	printInfo(info.frames, info.channels, info.samplerate);
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

	/*

	*/
	int chunksPerSecond = (info.samplerate / info.channels) / BUF_SIZE;
	int chunksPerWatermark = chunksPerSecond * watermarkInterval;
	long chunkCount = 0;
	while ((readCount = sf_read_double(infile, buffer, BUF_SIZE)))
	{
		/*increases total frames in the file instead of overwriting original value*/
		chunkCount++;
		sf_write_double(outfile, buffer, readCount);
		if (chunkCount % chunksPerWatermark == 0)
		{
			insertWatermark(buffer, readCount, info.channels);
			sf_write_double(outfile, buffer, readCount);
		}


	};

	/*
	Can only close SF_open once both reading/writing has been done
	if you close infile after the read, it's not able to copy the audio
	data from infile to write into outfile
	*/
	sf_close(infile);
	sf_close(outfile);

}

int decodeInsertion(int fileNumber)
{
	SF_INFO info;
	SNDFILE *infile;
	int readCount;
	static double buffer2[BUF_SIZE];


	char *inputFile = Files2[fileNumber - 1];

	//infile = sf_open(inputFile, SFM_READ, &info);
	if (!(infile = sf_open(inputFile, SFM_READ, &info)))
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

	while ((readCount = sf_read_double(infile, buffer2, BUF_SIZE)))
	{

		chunkCount++;
		retrieveWatermark(buffer2, readCount, info.channels, chunkCount);
	}

	sf_close(infile);
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
int counter = 0;
int counter1 = 0;
double  insertionWatermarks[] = { 0.0, 0.0, 9.0, 9.0, 1.0, 1.0, 9.0, 9.0, 1.0, 1.0, 9.0, 9.0,
0.6, 0.6, 0.7, 0.7, 0.8, 0.8, 0.9, 0.9 };
double scaleFactor = 2; // to reduce the volume of the watermark
double baseline = 0.0567;
double temp = (1 / 100) * 2 + 0.0567;

void insertWatermark(double *buffer, int count, int channels)
{

	int i, j, k;
	counter = counter;

	/*
	Sets j to be the first channel and while i is less than 1024/5, i += channels
	buffer[3] value is multiplied by 0, and set to 0
	this mutes that particular index value or frame
	this keeps going until i>=1024/5 and then the next channel is chosen where j = 2
	buffer[4] value is multiplied by 0 and set to 0
	this keeps going until i>=1024/5 where it calls back to the while loop in encodeInsertion
	*/

	int duplicates = 2;//dont duplicate the watermark

	for (i = 0; i < 20; i++){
		// insert duplicate of digit, in all channels
		for (j = 0; j < duplicates; j++){
			for (k = 0; k < channels; k++)
			{
				buffer[i*duplicates*channels + j*channels + k] = ((insertionWatermarks[i] / 100) * scaleFactor) + baseline;
				insertionBitsEncoded[counter] = ((insertionWatermarks[i] / 100) * scaleFactor) + baseline;
				counter++;
			}
		}
	}
}

void retrieveWatermark(double *buffer, int count, int channels, int chunkCount)
{
	//buffer comes in 1024
	int i, j, k;
	int watermarkCounter = 0;
	bool startOfWatermark = 0;
	int idxOfWatermarkStart = 0;
	int numbersToCount = 4 * channels;
	counter1 = counter1;

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
		if (fabs(((insertionWatermarks[0] / 100)* scaleFactor + baseline) - buffer[i]) <= 0.000031)
			//if (buffer[i] == 0)
		{

			watermarkCounter = 0;

			for (j = 0; j < numbersToCount; j++)
			{
				int idxToCheck = i + j;

				if (fabs(((insertionWatermarks[1] / 100)* scaleFactor + baseline) - buffer[idxToCheck]) <= 0.000031)
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
			//printf("Watermark position in file is: %d\nValue: %lf\n", BUF_SIZE*chunkCount + k, fabs(buffer[k]));
			insertionBitsDecoded[counter1] = fabs(buffer[k]);
			counter1++;
		}

	}
}

//used for getting accuracy of LSB algorithm
void getAccuracy(char a[], char b[])
{

	double total = 850;
	double score = 0;
	for (int j = 0; j < 850; j++)
	{
		if (a[j] == b[j])
		{
			score++;
		}

	}

	double accuracy = (score / total) * 100;
	printf("Decode accuracy is: %3.2f\n", accuracy);
}

//used for getting accuracy of Insertion algorithm
void insertionAccuracy(double a[], double b[])
{
	double total = 1360;
	double score = 0;
	for (int i = 0; i < 1360; i++){
		if (fabs(a[i] - b[i]) <= 0.0000166){
			score++;
		}
	}
	double accuracy = (score / total) * 100;
	printf("Decode accuracy is: %3.2f\n", accuracy);
}

//reads the two text files and writes the first 850 characrters of each file to different arrays
void moveToArray()
{
	
	FILE *fp1 = fopen(EmbedFile, "r");
	
	for (int i = 0; i < 850; i++){
		fscanf(fp1, "%c", &bitsEncoded[i]);
	}
	fclose(fp1);

	FILE *fp2 = fopen(EmbedFile2, "r");

	for (int i = 0; i < 850; i++){
		fscanf(fp2, "%c", &bitsDecoded[i]);
	}
	fclose(fp2);
}


//converts the each 4 bits into its decimal representation
void binaryToDecimal(int array[][COLUMNS])
{
	//int value[12];
	int i, j, k, index = 0;
	int decimal[12];

	//loop to iterate for the 10 sequence
	for (i = 0; i < 10; i++){
		//ignore the first 37 trigger bits
		for (j = 37; j < 85; j += 4){
			//save each 4 bits into an array
			int bits[] = { array[i][j], array[i][j + 1], array[i][j + 2], array[i][j + 3] };
			//connvert the 4 bits in the array into its decimal value
			decimal[index] = (bits[0] * 8) + (bits[1] * 4) + (bits[2] * 2) + (bits[3]);
			index++; //store the next next value
		}
		index = 0; //reset the index counter
		//print out the 12 values converted
		for (k = 0; k < 12; k++){
			printf("%d", decimal[k]);
		}
		printf("\n");
	}
	
}


//splits a 1D array of char consisting of 1s and 0s and saves the int value into a 2d array
void splitArray(){
	int i, j;
	for (i = 0; i < ROWS; i++)
	{
		for (j = 0; j < COLUMNS; j++)
		{
			decodedSequence[i][j] = bitsDecoded[i * COLUMNS + j] - '0';
		}
	}
}

//counts number of characters in the decoded file and subtracts number of bits encoded to get number of false positives
int countFalsePos()
{
	int falsePositives = 0;
	FILE *fp2 = fopen(EmbedFile2, "r");
	char ch;
	//counts the total number of bits decoded
	while ((ch = fgetc(fp2)) != EOF){
		falsePositives++;
	}
	falsePositives -= 850;
	return falsePositives;
}