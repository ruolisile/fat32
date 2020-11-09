#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>


typedef struct {
	int size;
	char **items;
} tokenlist;

//used to parse shell like commands
//reused from project 1
char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

//print disk info
void info(void);
void ls(char *DIR);

//global variables
unsigned int BPB_BytsPerSec = 0;
unsigned int BPB_SecPerClus = 0;
unsigned int BPB_RsvdSecCnt = 0;
unsigned int BPB_NumFATs = 0;
unsigned int BPB_TotSec32 = 0;
unsigned int BPB_FATSz32 = 0;
unsigned int BPB_RootClus = 0;
int FILE_IMG = 0;

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("Incorrect arguments; Usage: project3.o FILE_SYSTEM_IMAGE\n");
        return 1;
    }
    //open disk image
    FILE_IMG = open(argv[1], O_RDONLY);
    if(FILE_IMG < 0)
    {
        printf("ERROR: Failed to open disk image \n");
        return 1;
    }
	
    while (1) {
		printf("$ ");

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		//printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);

        char *command = (char *)malloc(strlen(tokens->items[0]) + 1);
        strcpy(command, tokens->items[0]);
 
        if(strcmp(command, "exit") == 0)
        {
            free(command);
            free(input);
            free_tokens(tokens);
			close(FILE_IMG);
            return 0;
        }
        else if (strcmp(command, "info") == 0)
		{
			info();
		}
        free(command);
		free(input);
		free_tokens(tokens);
	}

	return 0;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **) malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

tokenlist *get_tokens(char *input)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
}

void info(void)
{
    unsigned char buffer[512];
    lseek(FILE_IMG, 0, SEEK_SET);
	read(FILE_IMG, buffer, sizeof(buffer));
    //find the Bytes per sector
    //offset 11, size 2
    for(int i = 11+1; i >= 11; i--)
	{
        BPB_BytsPerSec = BPB_BytsPerSec << 8;
        BPB_BytsPerSec = BPB_BytsPerSec + buffer[i];
    }

	//sectors per cluster 
	BPB_SecPerClus = buffer[13];

	//find reserved sector count
	//offset 14 byts, size 2 bytes
	for (int i = 14+1; i >= 14; i--)
	{
		BPB_RsvdSecCnt = BPB_RsvdSecCnt << 8;
		BPB_RsvdSecCnt = BPB_RsvdSecCnt + buffer[i];
	}	

	//find number of Fats
	BPB_NumFATs = buffer[16];

	//find total sectors
	//offset 32, size 4 bytes
	for (int i = 32 + 3; i >= 32; i--)
	{
		BPB_TotSec32 = BPB_TotSec32 << 8;
		BPB_TotSec32 = BPB_TotSec32 + buffer[i];
	}
	
	//find FAT Size
	//offset 36 bytes, size 4 bytes
	for(int i = 36 + 3; i >= 36; i--)
	{
		BPB_FATSz32 = BPB_FATSz32 << 8;
		BPB_FATSz32 = BPB_FATSz32 + buffer[i];
	}

	//find root cluster
	//offset 44 bytes, size 4 bytes
	for (int i = 44 + 3; i >= 44; i--)
	{
		BPB_RootClus = BPB_RootClus << 8;
		BPB_RootClus = BPB_RootClus + buffer[i];
	}

	printf("Bytes Per Sector: %d\n", BPB_BytsPerSec);
	printf("Sectors per cluster: %d\n", BPB_SecPerClus);
	printf("Reseverd sector count: %d\n", BPB_RsvdSecCnt);
	printf("Number of FATS: %d\n", BPB_NumFATs);
	printf("Total sectors: %d\n", BPB_TotSec32);
	printf("FAT size: %d\n", BPB_FATSz32);
	printf("Root cluter: %d\n", BPB_RootClus);
}