#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>

typedef struct
{
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

//prototypes
void init(void); //initilized BPB values
void info(void);
void ls(tokenlist *tokens);
void lsName(unsigned long n); //print file names
struct DIRENTRY pathSearch(char *path);//search file
void size(tokenlist *tokens); //return size of a file
//DIR entry
struct DIRENTRY
{
	unsigned char DIR_Name[11];
	unsigned char DIR_Attr;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LstAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
	unsigned int DIR_FileSize;
} __attribute__((packed));
struct DIRENTRY directory[512];

//global variables
unsigned int BPB_BytsPerSec = 0;
unsigned int BPB_SecPerClus = 0;
unsigned int BPB_RsvdSecCnt = 0;
unsigned int BPB_NumFATs = 0;
unsigned int BPB_TotSec32 = 0;
unsigned int BPB_FATSz32 = 0;
unsigned int BPB_RootClus = 0;
unsigned int FirstDataSector = 0;
int file_img = 0;
unsigned long n; //cluster
int root;		 //root dir
int curr_clust;	 //current cluster
int pre_clust;	 //paraent cluster

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Incorrect arguments; Usage: project3.o FILE_SYSTEM_IMAGE\n");
		return 1;
	}
	//open disk image
	file_img = open(argv[1], O_RDONLY);
	if (file_img < 0)
	{
		printf("ERROR: Failed to open disk image \n");
		return 1;
	}
	init();
	while (1)
	{
		printf("$ ");

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		//printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);

		char *command = (char *)malloc(strlen(tokens->items[0]) + 1);
		strcpy(command, tokens->items[0]);

		if (strcmp(command, "exit") == 0)
		{
			free(command);
			free(input);
			free_tokens(tokens);
			close(file_img);
			return 0;
		}
		else if (strcmp(command, "info") == 0)
		{
			info();
		}
		else if(strcmp(command, "size") == 0)
		{
			size(tokens);	
		}
		else if (strcmp(command, "ls") == 0)
		{
			ls(tokens);
		}
		free(command);
		free(input);
		free_tokens(tokens);
	}

	return 0;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **)malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *)malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL)
	{
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

tokenlist *get_tokens(char *input)
{
	char *buf = (char *)malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL)
	{
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

void init(void)
{
	unsigned char buffer[512];
	lseek(file_img, 0, SEEK_SET);
	read(file_img, buffer, sizeof(buffer));
	//find the Bytes per sector
	//offset 11, size 2
	for (int i = 11 + 1; i >= 11; i--)
	{
		BPB_BytsPerSec = BPB_BytsPerSec << 8;
		BPB_BytsPerSec = BPB_BytsPerSec + buffer[i];
	}

	//sectors per cluster
	BPB_SecPerClus = buffer[13];

	//find reserved sector count
	//offset 14 byts, size 2 bytes
	for (int i = 14 + 1; i >= 14; i--)
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
	for (int i = 36 + 3; i >= 36; i--)
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

	//find first data sector
	FirstDataSector = BPB_RsvdSecCnt + BPB_NumFATs * BPB_FATSz32;

	//find root directory
	//root = ((BPB_RootClus - 2) * BPB_SecPerClus) + FirstDataSector;
	root = (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec);
	curr_clust = BPB_RootClus; //set current clust to root
}

void info()
{
	printf("Bytes Per Sector: %d\n", BPB_BytsPerSec);
	printf("Sectors per cluster: %d\n", BPB_SecPerClus);
	printf("Reseverd sector count: %d\n", BPB_RsvdSecCnt);
	printf("Number of FATS: %d\n", BPB_NumFATs);
	printf("Total sectors: %d\n", BPB_TotSec32);
	printf("FAT size: %d\n", BPB_FATSz32);
	printf("Root cluster: %d\n", BPB_RootClus);
}

void ls(tokenlist *tokens)
{
	if (tokens->size == 1)
	{
		n = curr_clust;
		lsName(n);
	}
	else if (tokens->size == 2)
	{
		if (strcmp(tokens->items[1], ".") == 0)
		{
			n = curr_clust;
			lsName(n);
		}
		if (strcmp(tokens->items[1], "..") == 0)
		{
			if (curr_clust == BPB_RootClus)
			{
				printf("ERROR: Already in root directory.");
			}
			else
			{
				n = pre_clust;
				lsName(n);
			}
		}
		else
		{
			struct DIRENTRY tempDir = pathSearch(tokens->items[1]);	
			if(tempDir.DIR_Name[0] == 0x0)
			{
				printf("ERROR: File/directory not found");
			}
			else if (tempDir.DIR_Attr == 0x10)
			{
				printf("%s is a dir\n", tokens->items[1]);
				//get clust 
				n = tempDir.DIR_FstClusLO;
				lsName(n);
			}
			else //if a file, print file name
			{
				printf("%s", tokens->items[1]);
			}
		}
		
	}
	else
	{
		printf("ERROR: Invalid number of arguments");
	}
	printf("\n");
	//low word give the first cluster n
}

void lsName(unsigned long n)
{
	unsigned int Offset;
	struct DIRENTRY tempDir;

	Offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
	lseek(file_img, Offset, SEEK_SET);
	read(file_img, &tempDir, 32);

	while (tempDir.DIR_Name[0] != 0x0)
	{

		if (tempDir.DIR_Attr != 0xF && tempDir.DIR_Name[0] != 0x0 && tempDir.DIR_Name[0] != 0x2E)
		{
			int j;
			for (j = 0; j < 11; j++)
			{
				if (tempDir.DIR_Name[j] != ' ')
					printf("%c", tempDir.DIR_Name[j]);
			}
			printf("    ");
		}
		Offset += 32;
		lseek(file_img, Offset, SEEK_SET);
		read(file_img, &tempDir, 32);
	}
}

struct DIRENTRY pathSearch(char *path)
{
	char pathUp[12];
	for (int i = 0; i < strlen(path); i++)
	{
		pathUp[i] = toupper(path[i]);
	}
	for (int i = strlen(path); i < 11; i++)
	{
		pathUp[i] = ' ';
	}
	pathUp[11] = '\0';

	unsigned int Offset;
	struct DIRENTRY tempDir;
	
	n = curr_clust;
	Offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
	lseek(file_img, Offset, SEEK_SET);
	read(file_img, &tempDir, 32);

	while (tempDir.DIR_Name[0] != 0x0)
	{
		char name[12];
		if (tempDir.DIR_Attr != 0xF && tempDir.DIR_Name[0] != 0x0 && tempDir.DIR_Name[0] != 0x2E)
		{
			int j;
			for (j = 0; j < 11; j++)
			{
				name[j] = tempDir.DIR_Name[j];
			}
			name[11] = '\0';
			if (strcmp(name, pathUp) == 0)
			{
				printf("Found\n");
				return tempDir;
			}
			
		}
		Offset += 32;
		lseek(file_img, Offset, SEEK_SET);
		read(file_img, &tempDir, 32);
	}
	tempDir.DIR_Name[0] == 0x0;
	return tempDir;//if path not found	
	
}

void size(tokenlist *tokens)
{
	if(tokens->size < 2)
	{
		printf("ERROR: Too few arguments. Usage: size <file>\n");
	}
	else
	{
		struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
		if(tempDir.DIR_Name[0] == 0x0)
		{
			printf("ERROR: File not found\n");
		}
		else
		{
			printf("%d\n", tempDir.DIR_FileSize);
		}
		
	}
	
}