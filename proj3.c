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
void lsName(unsigned long n);			//print file names
struct DIRENTRY pathSearch(char *path); //search file
unsigned int size(tokenlist *tokens);	//return size of a file
void cd(tokenlist *tokens);				//cd
unsigned int findEmptyClus(void);		//find next empty clus
void create(char *FILE);				//create file
void mkdir(tokenlist *tokens);
void openFile(char *FILE, char *mode); //open file
//add to open file list
void addOpenFile(unsigned int clust, char *FILE, char *mode);
struct openFile *isFileOpened(unsigned int clust, char *name); // if file is open
void freeFile(void);										   //free opened files
void closeFile(char *FILE);
void removeOpenFile(unsigned int clust, char *FILE);
void printlist();				   //print open file list
void lseekFile(tokenlist *tokens); //lseek an open file
char *readFile(char *FILE, unsigned int readBytes);
void writeFile(char *FILE, unsigned int writeBytes, char *string);
unsigned int updateClust(unsigned int clust);
void writeToFile(char *string, unsigned int start, unsigned int size);
void rmFile(char *FILE);
void cpFile(tokenlist *tokens);
void mvFile(char *FILE, char *dest);
int isLastEntry(unsigned int src_offset, unsigned int curr_offset);
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

//struct for open files
struct openFile
{
	unsigned char name[11];
	unsigned int firstClus;
	unsigned int offSet;
	unsigned char mode[2];
	struct openFile *next;
};

struct openFile *head = NULL;
struct openFile *current = NULL;

//global variables
unsigned int BPB_BytsPerSec = 0;
unsigned int BPB_SecPerClus = 0;
unsigned int BPB_RsvdSecCnt = 0;
unsigned int BPB_NumFATs = 0;
unsigned int BPB_TotSec32 = 0;
unsigned int BPB_FATSz32 = 0;
unsigned int BPB_RootClus = 0;
unsigned int FirstDataSector = 0;
unsigned int BytsPerClus = 0;
int file_img = 0;
unsigned long n; //cluster
int root;		 //root dir
int curr_clust;	 //first clust for current dir
int pare_clust;	 //first clust for paraent cluster

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Incorrect arguments; Usage: project3.o FILE_SYSTEM_IMAGE\n");
		return 1;
	}
	//open disk image
	file_img = open(argv[1], O_RDWR);
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
			freeFile();
			free_tokens(tokens);
			close(file_img);
			return 0;
		}
		else if (strcmp(command, "info") == 0)
		{
			info();
		}
		else if (strcmp(command, "size") == 0)
		{
			int fileSize = size(tokens);
			if (fileSize >= 0)
			{
				printf("%d\n", fileSize);
			}
		}
		else if (strcmp(command, "ls") == 0)
		{
			ls(tokens);
		}
		else if (strcmp(command, "cd") == 0)
		{
			cd(tokens);
		}
		else if (strcmp(command, "create") == 0)
		{
			create(tokens->items[1]);
		}
		else if (strcmp(command, "mkdir") == 0)
		{
			mkdir(tokens);
		}
		else if (strcmp(command, "open") == 0)
		{
			if (tokens->size != 3)
			{
				printf("ERROR: Incorrect number of arguments. Usage: open <file> <mode>\n");
			}
			else
			{
				openFile(tokens->items[1], tokens->items[2]);
			}
		}
		else if (strcmp(command, "close") == 0)
		{
			if (tokens->size != 2)
			{
				printf("ERROR: Incorrect number of arguments. Usage: close <file>\n");
			}
			else
			{
				closeFile(tokens->items[1]);
			}
		}
		else if (strcmp(command, "lseek") == 0)
		{
			lseekFile(tokens);
		}

		else if (strcmp(command, "print") == 0)
		{
			printlist();
		}
		else if (strcmp(command, "read") == 0)
		{
			if (tokens->size != 3)
			{
				printf("ERROR: Inconrrect number of arguments. Usage: read <file> <size>\n");
			}
			else
			{
				unsigned int readBytes = strtoul(tokens->items[2], NULL, 10);
				char *temp = readFile(tokens->items[1], readBytes);
				if (temp != NULL)
				{
					printf("%s\n", temp);
					free(temp);
				}
			}
		}
		else if (strcmp(command, "write") == 0)
		{
			if (tokens->size != 4)
			{
				printf("ERROR: Incorrect number of arguments. Usage: write <file> <size> <\"STRING\">\n");
			}
			else
			{
				unsigned int writeBytes = strtoul(tokens->items[2], NULL, 10);
				writeFile(tokens->items[1], writeBytes, tokens->items[3]);
			}
		}
		else if (strcmp(command, "rm") == 0)
		{
			if (tokens->size != 2)
			{
				printf("ERROR: Incorrect number of arguments. Usage: rm <file>\n");
			}
			else
			{
				rmFile(tokens->items[1]);
			}
		}
		else if (strcmp(command, "cp") == 0)
		{
			cpFile(tokens);
		}
		else if (strcmp(command, "mv") == 0)
		{
			if (tokens->size != 3)
			{
				printf("ERROR: Invalid number of arguments. Usage: mv <from> <to>\n");
			}
			else
			{
				mvFile(tokens->items[1], tokens->items[2]);
			}
		}
		else
		{
			printf("ERROR: Invalid command\n");
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
		if (tok[0] == '\"' && tok[strlen(tok) - 1] == '\"')
		{
			char tempTok[strlen(tok) - 2];
			strncpy(tempTok, tok + 1, strlen(tok) - 2);
			add_token(tokens, tempTok);
			tok = strtok(NULL, " ");
		}
		else if (tok[0] == '\"')
		{
			char tempTok[strlen(input) + 1];
			memset(tempTok, '\0', strlen(input) + 1);
			strncpy(tempTok, tok + 1, strlen(tok) - 1);
			tok = strtok(NULL, " ");
			while (tok[strlen(tok) - 1] != '\"')
			{
				strcat(tempTok, " ");
				strcat(tempTok, tok);
				tok = strtok(NULL, " ");
			}
			if (tok[strlen(tok) - 1] == '\"')
			{
				strcat(tempTok, " ");
				char end[strlen(tok) + 1];
				memset(end, '\0', strlen(tok) + 1);
				strncpy(end, tok, strlen(tok) - 1);
				strcat(tempTok, end);
			}

			add_token(tokens, tempTok);
			tok = strtok(NULL, " ");
		}
		else
		{
			add_token(tokens, tok);
			tok = strtok(NULL, " ");
		}
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
	BytsPerClus = BPB_BytsPerSec * BPB_SecPerClus;
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
		else if (strcmp(tokens->items[1], "..") == 0)
		{
			if (curr_clust == BPB_RootClus)
			{
				printf("ERROR: Already in root directory.\n");
			}
			else
			{
				n = pare_clust;
				lsName(n);
			}
		}
		else
		{
			struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
			if (tempDir.DIR_Name[0] == 0x0)
			{
				printf("ERROR: File/directory not found\n");
			}
			else if (tempDir.DIR_Attr == 0x10)
			{
				//get clust
				n = tempDir.DIR_FstClusHI << 16;
				n += tempDir.DIR_FstClusLO;
				lsName(n);
			}
			else //if a file, print file name
			{
				printf("%s\n", tokens->items[1]);
			}
		}
	}
	else
	{
		printf("ERROR: Invalid number of arguments\n");
	}
}

void lsName(unsigned long n)
{
	unsigned int Offset;
	struct DIRENTRY tempDir;

	Offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
	unsigned int nOffset = Offset;

	lseek(file_img, Offset, SEEK_SET);
	read(file_img, &tempDir, 32);

	while (tempDir.DIR_Name[0] != 0x0 && Offset <= 0xFFFFFF8)
	{

		if (tempDir.DIR_Attr != 0xF && tempDir.DIR_Name[0] != 0x0 && tempDir.DIR_Name[0] != 0xe5)
		{
			int j;
			for (j = 0; j < 11; j++)
			{
				if (tempDir.DIR_Name[j] != ' ')
					printf("%c", tempDir.DIR_Name[j]);
			}
			printf("\n");
		}
		Offset += 32;

		if (Offset > nOffset + BytsPerClus)
		{
			//if reach the end of this cluster
			//move to next cluster
			unsigned int nextClus;
			unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + n * 4;
			lseek(file_img, fatOffset, SEEK_SET);
			read(file_img, &nextClus, 4);
			n = nextClus;
			//printf("Next clust is %d\n", n);
			Offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
			nOffset = Offset;
		}

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
	unsigned int nOffset = Offset;
	lseek(file_img, Offset, SEEK_SET);
	read(file_img, &tempDir, 32);

	while (tempDir.DIR_Name[0] != 0x0 && Offset < 0xFFFFFF8)
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
				return tempDir;
			}
		}
		Offset += 32;
		if (Offset > nOffset + BytsPerClus)
		{
			//if reach the end of this cluster
			//move to next cluster
			unsigned int nextClus;
			unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + n * 4;
			lseek(file_img, fatOffset, SEEK_SET);
			read(file_img, &nextClus, 4);
			n = nextClus;
			//printf("Next clust is %d\n", n);
			Offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
			nOffset = Offset;
		}
		lseek(file_img, Offset, SEEK_SET);
		read(file_img, &tempDir, 32);
	}
	tempDir.DIR_Name[0] == 0x0;
	//return dir entry with name started at 0 if not found
	return tempDir; //if path not found
}

unsigned int size(tokenlist *tokens)
{
	if (tokens->size < 2)
	{
		printf("ERROR: Too few arguments. Usage: size <file>\n");
	}
	else
	{
		struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
		if (tempDir.DIR_Name[0] == 0x0)
		{
			printf("ERROR: File not found\n");
			return -1;
		}
		else
		{
			return tempDir.DIR_FileSize;
		}
	}
}
//only work for directory with no duplicate names
void cd(tokenlist *tokens)
{
	if (tokens->size == 1)
	{
		curr_clust = BPB_RootClus; //cd to root
		pare_clust = BPB_RootClus;
	}
	else if (strcmp(tokens->items[1], ".") == 0)
	{
		curr_clust = curr_clust;
		pare_clust = pare_clust;
	}
	else if (strcmp(tokens->items[1], "..") == 0)
	{
		if (curr_clust == BPB_RootClus)
		{
			printf("ERROR: Already in root directory\n");
		}
	}
	else
	{
		struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
		if (tempDir.DIR_Name[0] == 0x0)
		{
			printf("ERROR: Directory not found\n");
		}
		else if (tempDir.DIR_Attr != 0x10) //not a dir
		{
			printf("ERROR: Not a directory\n");
		}
		else
		{
			pare_clust = curr_clust;
			curr_clust = tempDir.DIR_FstClusHI << 16;
			curr_clust += tempDir.DIR_FstClusLO;
		}
	}
}

void create(char *FILE)
{
	struct DIRENTRY tempDir = pathSearch(FILE);
	if (tempDir.DIR_Name[0] != 0x0)
	{
		printf("ERROR: File already exists\n");
	}
	else
	{
		struct DIRENTRY newFile;
		unsigned int emptClus = findEmptyClus();

		unsigned int lastClus = 0xFFFFFF8;
		lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + emptClus * 4, SEEK_SET);
		//set emptclus to last cluster
		write(file_img, &lastClus, 4);
		//copy name
		char *fileName = FILE;
		for (int i = 0; i < strlen(fileName); i++)
		{
			newFile.DIR_Name[i] = toupper(fileName[i]);
		}
		for (int i = strlen(fileName); i < 11; i++)
		{
			newFile.DIR_Name[i] = ' ';
		}

		newFile.DIR_Attr = 0x20;
		newFile.DIR_NTRes = 0;
		newFile.DIR_CrtTimeTenth = 0;
		newFile.DIR_CrtTime = 0;
		newFile.DIR_CrtDate = 0;
		newFile.DIR_LstAccDate = 0;
		newFile.DIR_FstClusHI = (emptClus >> 16) & 0xFFFF;
		newFile.DIR_WrtTime = 0;
		newFile.DIR_WrtDate = 0;
		newFile.DIR_FstClusLO = 0xFFFF & emptClus;
		newFile.DIR_FileSize = 0;

		struct DIRENTRY temp;
		unsigned int offset;
		offset = BPB_BytsPerSec * (FirstDataSector + (curr_clust - 2) * BPB_SecPerClus);
		unsigned int n = curr_clust;
		unsigned int nOffset = offset;
		while (offset < 0xFFFFFF8)
		{
			//printf("offset is %ld\n", offset);

			lseek(file_img, offset, SEEK_SET);
			read(file_img, &temp, 32);
			if (temp.DIR_Name[0] == 0x0 | temp.DIR_Name[0] == 0xe5)
			{
				lseek(file_img, offset, SEEK_SET);
				int a = write(file_img, &newFile, 32);
				break;
			}
			offset += 32;
			if (offset > nOffset + BytsPerClus)
			{
				//if reach the end of this cluster, move to next cluster
				unsigned int nextClus;
				unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + n * 4;
				lseek(file_img, fatOffset, SEEK_SET);
				read(file_img, &nextClus, 4);
				n = nextClus;
				//printf("Next clust is %d\n", n);
				offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
				nOffset = offset;
			}
		}
	}
}

unsigned int findEmptyClus()
{
	unsigned int tempClus = BPB_RootClus;
	unsigned int clus = tempClus;
	while (clus != 0)
	{
		tempClus++;
		lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + tempClus * 4, SEEK_SET);
		read(file_img, &clus, 4);
	}
	return tempClus;
}

void mkdir(tokenlist *tokens)
{
	if (tokens->size < 2)
	{
		printf("ERROR: Too few arguments. Usage mkdir <directory>\n");
	}
	else
	{
		struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
		if (tempDir.DIR_Name[0] != 0x0 && tempDir.DIR_Attr == 0x10)
		{
			printf("ERROR: Directory already exists\n");
		}
		else
		{
			struct DIRENTRY newDir;
			unsigned int emptClus = findEmptyClus();

			unsigned int lastClus = 0x0FFFFF8;
			lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + emptClus * 4, SEEK_SET);
			//set emptclus to last cluster
			write(file_img, &lastClus, 4);
			//copy name
			char *fileName = tokens->items[1];
			for (int i = 0; i < strlen(fileName); i++)
			{
				newDir.DIR_Name[i] = toupper(fileName[i]);
			}
			for (int i = strlen(fileName); i < 11; i++)
			{
				newDir.DIR_Name[i] = ' ';
			}

			newDir.DIR_Attr = 0x10;
			newDir.DIR_NTRes = 0;
			newDir.DIR_CrtTimeTenth = 0;
			newDir.DIR_CrtTime = 0;
			newDir.DIR_CrtDate = 0;
			newDir.DIR_LstAccDate = 0;
			newDir.DIR_FstClusHI = (emptClus >> 16) & 0xFFFF;
			newDir.DIR_WrtTime = 0;
			newDir.DIR_WrtDate = 0;
			newDir.DIR_FstClusLO = 0xFFFF & emptClus;
			newDir.DIR_FileSize = 0;

			struct DIRENTRY temp;
			unsigned int offset;
			offset = BPB_BytsPerSec * (FirstDataSector + (curr_clust - 2) * BPB_SecPerClus);
			unsigned int nOffset = offset;
			unsigned int n = curr_clust;
			while (offset < 0xFFFFFF8)
			{

				lseek(file_img, offset, SEEK_SET);
				read(file_img, &temp, 32);
				if (temp.DIR_Name[0] == 0x0)
				{
					lseek(file_img, offset, SEEK_SET);
					int a = write(file_img, &newDir, 32);
					break;
				}
				offset += 32;

				if (offset > nOffset + BytsPerClus)
				{
					//if reach the end of this cluster, move to next cluster
					unsigned int nextClus;
					unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + n * 4;
					lseek(file_img, fatOffset, SEEK_SET);
					read(file_img, &nextClus, 4);
					n = nextClus;
					//printf("Next clust is %d\n", n);
					offset = BPB_BytsPerSec * (FirstDataSector + (n - 2) * BPB_SecPerClus);
					nOffset = offset;
				}
			}

			struct DIRENTRY pareDir;
			strncpy(pareDir.DIR_Name, "..           ", 11);
			pareDir.DIR_Attr = 0x10;
			pareDir.DIR_NTRes = 0;
			pareDir.DIR_CrtTimeTenth = 0;
			pareDir.DIR_CrtTime = 0;
			pareDir.DIR_CrtDate = 0;
			pareDir.DIR_LstAccDate = 0;
			pareDir.DIR_FstClusHI = (curr_clust >> 16) & 0xFFFF;
			pareDir.DIR_WrtTime = 0;
			pareDir.DIR_WrtDate = 0;
			pareDir.DIR_FstClusLO = 0xFFFF & curr_clust;
			pareDir.DIR_FileSize = 0;
			//add parent entry
			offset = BPB_BytsPerSec * (FirstDataSector + (emptClus - 2) * BPB_SecPerClus);
			lseek(file_img, offset, SEEK_SET);
			//add current dir entry
			strncpy(temp.DIR_Name, ".           ", 11);
			write(file_img, &temp, 32);
			write(file_img, &pareDir, 32);

			//add empty dir entry
			struct DIRENTRY emptEntry;
			emptEntry.DIR_Name[0] = 0x0;
			write(file_img, &emptEntry, 32);
		}
	}
}

void openFile(char *FILE, char *mode)
{ //r. w. rw. wr

	struct DIRENTRY file = pathSearch(FILE);
	if (file.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
	}
	else if (file.DIR_Attr & 0x10)
	{

		printf("ERROR: %s is a direcotry. Cannot open a directory\n", FILE);
	}
	else if (strcmp(mode, "rw") != 0 && strcmp(mode, "w") != 0 &&
			 strcmp(mode, "wr") != 0 && strcmp(mode, "r") != 0)
	{
		printf("ERROR: Invalid mode. Valid modes are r, w, rw, wr\n");
	}
	else if ((strcmp(mode, "rw") == 0 | strcmp(mode, "w") == 0 |
			  strcmp(mode, "wr") == 0) &&
			 (file.DIR_Attr & 0x01))
	{
		printf("ERROR: Ths file is read only\n");
	}
	else
	{
		unsigned int clust = file.DIR_FstClusHI << 16;
		clust += file.DIR_FstClusLO;
		unsigned int offSet = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
		if (isFileOpened(clust, FILE) != NULL)
		{
			printf("ERROR: Fill already open\n");
		}
		else
		{
			addOpenFile(clust, FILE, mode);
			printf("Open %s in %s mode\n", FILE, mode);
		}
	}
}

void addOpenFile(unsigned int clust, char *FILE, char *mode)
{
	struct openFile *file_ptr = (struct openFile *)malloc(sizeof(struct openFile));
	file_ptr->firstClus = clust;
	file_ptr->offSet = 0;
	strcpy(file_ptr->name, FILE);
	strcpy(file_ptr->mode, mode);
	file_ptr->next = head;
	head = file_ptr;
}

struct openFile *isFileOpened(unsigned int clust, char *name)
{
	struct openFile *temp = head;
	if (head == NULL)
	{
		return NULL; //not in the list
	}
	while (temp->firstClus != clust | strcmp(temp->name, name) != 0)
	{
		if (temp->next == NULL)
		{
			return NULL; //not in the list
		}
		else
		{
			temp = temp->next;
		}
	}
	return temp;
}
void freeFile()
{
	struct openFile *current = head;
	struct openFile *temp;
	if (head != NULL)
	{
		while (current->next != NULL)
		{
			temp = current->next;
			free(current);
			current = temp;
		}
		free(current);
	}
}

void closeFile(char *FILE)
{

	struct DIRENTRY file = pathSearch(FILE);
	if (file.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
	}
	else if (file.DIR_Attr & 0x10)
	{

		printf("ERROR: %s is a direcotry. Cannot open a directory\n", FILE);
	}
	else
	{
		unsigned int clust = file.DIR_FstClusHI << 16;
		clust += file.DIR_FstClusLO;
		if (isFileOpened(clust, FILE) == NULL)
		{
			printf("ERROR: Fill is not open\n");
		}
		else
		{
			removeOpenFile(clust, FILE);
		}
	}
}

void removeOpenFile(unsigned int clust, char *FILE)
{
	struct openFile *current = head;
	struct openFile *previous = NULL;
	if (head == NULL)
	{
		return;
	}
	while (current->firstClus != clust | strcmp(current->name, FILE) != 0)
	{
		previous = current;
		current = current->next;
	}
	//if found a match and delete, update link
	if (current == head)
	{
		head = head->next;
	}
	else
	{
		previous->next = current->next;
	}
}

void printlist()
{
	struct openFile *ptr = head;
	while (ptr != NULL)
	{
		printf("File %s is open in %s mode; Clust is %d, Offset is %d\n", ptr->name, ptr->mode, ptr->firstClus, ptr->offSet);
		ptr = ptr->next;
	}
}

void lseekFile(tokenlist *tokens)
{
	if (tokens->size < 3)
	{
		printf("ERROR: Too few arguments. Usage: lseek <file> <size>\n");
	}
	else
	{
		struct DIRENTRY file = pathSearch(tokens->items[1]);
		if (file.DIR_Name[0] == 0x0)
		{
			printf("ERROR: File not exits\n");
		}
		else
		{
			unsigned int clust = file.DIR_FstClusHI << 16;
			clust += file.DIR_FstClusLO;
			struct openFile *temp = isFileOpened(clust, tokens->items[1]);
			if (temp == NULL)
			{
				printf("ERROR: File is not open\n");
			}
			else
			{
				unsigned int offset = strtoul(tokens->items[2], NULL, 10);
				if (offset > size(tokens))
				{
					printf("ERROR: Offset cannot greater than file size. \n");
				}
				else
				{
					temp->offSet = offset;
				}
			}
		}
	}
}

char *readFile(char *FILE, unsigned int readBytes)
{
	struct DIRENTRY file = pathSearch(FILE);
	if (file.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
		return NULL;
	}
	else if (file.DIR_Attr == 0X10)
	{
		printf("ERROR: Cannot read a directory\n");
		return NULL;
	}
	else
	{
		unsigned int clust = file.DIR_FstClusHI << 16;
		clust += file.DIR_FstClusLO;
		struct openFile *temp = isFileOpened(clust, FILE);
		if (temp == NULL)
		{
			printf("ERROR: File is not open. Please open the file first\n");
			return NULL;
		}
		else if (strcmp(temp->mode, "w") == 0)
		{
			printf("ERROR: This file is open in write mode only\n");
			return NULL;
		}
		else
		{
			unsigned int byteToRead = readBytes;
			unsigned int tempOffset = temp->offSet;
			char *outputString = (char *)malloc(byteToRead + 1);
			memset(outputString, '\0', byteToRead);
			while (tempOffset > BytsPerClus)
			{ //update clus number and temp offset
				unsigned int nextClust;
				unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4;
				lseek(file_img, fatOffset, SEEK_SET);
				read(file_img, &nextClust, 4);
				clust = nextClust;
				unsigned int offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
				lseek(file_img, offset, SEEK_SET);
				tempOffset -= BytsPerClus;
			}
			//store current clust number
			unsigned int offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
			lseek(file_img, offset + tempOffset, SEEK_SET);
			if (byteToRead > (file.DIR_FileSize - temp->offSet))
			{
				byteToRead = (file.DIR_FileSize - temp->offSet);
			}
			//read current clust first
			if (byteToRead >= (BytsPerClus - tempOffset))
			{
				unsigned char data[(BytsPerClus - tempOffset) + 1];
				int temp = read(file_img, &data, (BytsPerClus - tempOffset));
				data[(BytsPerClus - tempOffset)] = '\0';
				strcat(outputString, data);

				byteToRead -= (BytsPerClus - tempOffset);
				//move to next clust
				unsigned int nextClust;
				unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4;
				lseek(file_img, fatOffset, SEEK_SET);
				read(file_img, &nextClust, 4);
				clust = nextClust;

				offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
				lseek(file_img, offset, SEEK_SET);
			}
			while (byteToRead > 0)
			{

				if (byteToRead > BytsPerClus)
				{
					unsigned char data[BPB_BytsPerSec + 1];
					int temp = read(file_img, &data, BPB_BytsPerSec);
					data[BPB_BytsPerSec] = '\0';
					// 	printf("%s", data);

					strcat(outputString, data);

					byteToRead -= BPB_BytsPerSec;
					//move to next clust
					unsigned int nextClust;
					unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4;
					lseek(file_img, fatOffset, SEEK_SET);
					read(file_img, &nextClust, 4);
					clust = nextClust;
					offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
					lseek(file_img, offset, SEEK_SET);
				}
				else
				{
					unsigned char data[byteToRead];
					int temp = read(file_img, &data, byteToRead);
					data[byteToRead] = '\0';

					strcat(outputString, data);

					byteToRead = 0;
				}
			}
			return outputString;
		}
	}
}

void writeFile(char *FILE, unsigned int writeBytes, char *writeString)
{

	struct DIRENTRY file = pathSearch(FILE);
	if (file.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
		return;
	}
	else if (file.DIR_Attr == 0X10)
	{
		printf("ERROR: Cannot write to a directory\n");
		return;
	}
	else
	{
		unsigned int clust = file.DIR_FstClusHI << 16;
		clust += file.DIR_FstClusLO;
		struct openFile *temp = isFileOpened(clust, FILE);
		if (temp == NULL)
		{
			printf("ERROR: File is not open. Please open the file first\n");
			return;
		}
		else if (strcmp(temp->mode, "r") == 0)
		{
			printf("ERROR: This file is open in read mode only\n");
			return;
		}
		//get starting offset of dir entry of this write file
		//for later update in size
		unsigned int dirEntry_offset = lseek(file_img, 0, SEEK_CUR) - 32;
		printf("%x\n", dirEntry_offset);

		//unsigned int writeBytes = strtoul(tokens->items[2], NULL, 10);
		unsigned int byteToWrite = writeBytes;
		// //get write string, 2 for quotation mark, 1 for null
		unsigned char string[writeBytes + 2];
		memset(string, '\0', writeBytes + 2);
		strcat(string, writeString);
		// for (int i = 3; i < tokens->size - 1; i++)
		// {
		// 	strcat(string, tokens->items[i]);
		// 	strcat(string, " ");
		// }
		// //remove "" at the end of string
		// unsigned char lastToken[strlen(tokens->items[tokens->size - 1])];
		// strncpy(lastToken, tokens->items[tokens->size - 1], strlen(tokens->items[tokens->size - 1]));
		// lastToken[strlen(tokens->items[tokens->size - 1]) - 1] = '\0';
		// strcat(string, lastToken);

		printf("write string is %s\n ", string);

		//set current clust and offset corespondingly
		unsigned int tempOffset = temp->offSet;
		while (tempOffset > BytsPerClus)
		{ //update clust number and temp offset
			unsigned int nextClust;
			unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4;
			lseek(file_img, fatOffset, SEEK_SET);
			read(file_img, &nextClust, 4);
			clust = nextClust;
			unsigned int offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
			lseek(file_img, offset, SEEK_SET);
			tempOffset -= BytsPerClus;
		}
		printf("current clus is %d; tempoffset is %d\n", clust, tempOffset);
		//set pointer to correct place
		unsigned int offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
		lseek(file_img, offset + tempOffset, SEEK_SET);
		printf("pointer is at %x\n", offset + tempOffset);
		//update file size, and allocate more cluster
		if (byteToWrite > (file.DIR_FileSize - temp->offSet))
		{
			unsigned int currNumClust = file.DIR_FileSize / BytsPerClus;
			if (file.DIR_FileSize % BytsPerClus > 0)
			{
				currNumClust++;
			}
			unsigned int finalClust = (temp->offSet + writeBytes) / BytsPerClus;
			if ((temp->offSet + writeBytes) % BytsPerClus > 0)
			{
				finalClust++;
			}
			unsigned int extraClust = finalClust - currNumClust;
			printf("Extra clust is %d, current clust is %d\n", extraClust, currNumClust);
			if (extraClust == 0)
			{
				//write on current clust is enough
				writeToFile(string, 0, byteToWrite);
				byteToWrite -= byteToWrite;
				printf("pointer is at %x\n", offset + tempOffset);
			}
			else
			{
				//write on current clust first
				if (byteToWrite > (BytsPerClus - tempOffset))
				{
					writeToFile(string, 0, BytsPerClus - tempOffset);
					byteToWrite -= (BytsPerClus - tempOffset);
				}
				else
				{
					writeToFile(string, 0, byteToWrite);
					byteToWrite -= byteToWrite;
				}
				printf("pointer is at %x\n", offset + tempOffset);
				while (byteToWrite > 0)
				{ //Allocate new clust
					//link next clus
					//lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4, SEEK_SET);
					//unsigned int test;
					//read(file_img, &test, 4);
					//printf("before update %d\n", test);
					unsigned int nextclust;
					nextclust = updateClust(clust);
					printf("next clust is %d\n", nextclust);
					//if last clust, allocate new clust
					if (nextclust >= 0xFFFF08)
					{
						nextclust = findEmptyClus();
						lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4, SEEK_SET);
						write(file_img, &nextclust, 4);
						printf("allocate new class %d\n", nextclust);

						unsigned int lastClus = 0xFFFFFFF;
						lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + nextclust * 4, SEEK_SET);
						//set last cluster
						write(file_img, &lastClus, 4);
					}

					//printf("empty clust is %d; write \n", nextclust);
					clust = nextclust; //move to next clust
					//lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4, SEEK_SET);

					//read(file_img, &test, 4);
					//printf("after update %d\n", test);

					offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
					lseek(file_img, offset, SEEK_SET);
					//printf("pointer at %x\n", offset);
					unsigned int start = writeBytes - byteToWrite;
					//printf("Start is %d\n", start);
					if (byteToWrite >= BytsPerClus)
					{
						writeToFile(string, start, BytsPerClus);
						byteToWrite -= BytsPerClus;
					}
					else
					{
						writeToFile(string, start, byteToWrite);
						byteToWrite -= byteToWrite;
					}
				}
			}
			//update file size in dir entry
			file.DIR_FileSize = temp->offSet + writeBytes;
			lseek(file_img, dirEntry_offset, SEEK_SET);
			write(file_img, &file, sizeof(file));
		}
		else if (byteToWrite > 0)
		{
			if (byteToWrite > (BytsPerClus - tempOffset))
			{
				//write on current clust first
				writeToFile(string, 0, BytsPerClus - tempOffset);
				byteToWrite -= (BytsPerClus - tempOffset);

				clust = updateClust(clust);
				offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
				lseek(file_img, offset, SEEK_SET);
				printf("pointer is at %x\n", offset + tempOffset);
			}
			else
			{
				//write on current clust first
				writeToFile(string, 0, byteToWrite);
				byteToWrite -= byteToWrite;
			}

			while (byteToWrite > 0)
			{
				unsigned int startByte = writeBytes - byteToWrite;
				printf("start byt is %d\n", startByte);
				if (byteToWrite >= BytsPerClus)
				{
					writeToFile(string, startByte, BytsPerClus);
					byteToWrite -= BytsPerClus;

					clust = updateClust(clust);
					offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
					lseek(file_img, offset, SEEK_SET);
					printf("pointer is at %x\n", offset + tempOffset);
				}
				else
				{
					writeToFile(string, startByte, byteToWrite);
					byteToWrite -= byteToWrite;
					printf("pointer is at %x\n", offset + tempOffset);
				}
			}
		}
	}
}

unsigned int updateClust(unsigned int clust)
{
	//move to starting offset of next clust
	unsigned int nextClust;
	unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4;
	lseek(file_img, fatOffset, SEEK_SET);
	read(file_img, &nextClust, 4);
	return nextClust;
}

void writeToFile(char *string, unsigned int start, unsigned int size)
{
	unsigned char tempString[size + 1];
	memset(tempString, '\0', size + 1);
	strncpy(tempString, string + start, size);
	int temp = write(file_img, &tempString, size);
	printf("write %d bytes; %s\n", temp, tempString);
}

void rmFile(char *FILE)
{

	struct DIRENTRY file = pathSearch(FILE);

	if (file.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
		return;
	}
	else if (file.DIR_Attr == 0X10)
	{
		printf("ERROR: Cannot remove a directory\n");
		return;
	}
	//get starting offset of dir entry of this write file
	//for later update
	unsigned int dirEntry_offset = lseek(file_img, 0, SEEK_CUR) - 32;
	//get starting offset of current working clust
	unsigned int curr_offset = BPB_BytsPerSec * (FirstDataSector + (curr_clust - 2) * BPB_SecPerClus);
	//get the first clust of the file content
	unsigned int clust = file.DIR_FstClusHI << 16;
	clust += file.DIR_FstClusLO;
	printf("Dir offset is %x, clust is %d\n", dirEntry_offset, clust);
	struct openFile *temp = isFileOpened(clust, FILE);
	if (temp != NULL)
	{
		printf("ERROR: File is open. Please close the file before remove.\n");
		return;
	}
	unsigned int nextClust = updateClust(clust);
	//number of clust needs to be free
	unsigned int numClust = file.DIR_FileSize / BytsPerClus;
	if (file.DIR_FileSize % BytsPerClus > 0)
	{
		numClust++;
	}
	for (int i = 0; i < numClust; i++)
	{
		unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4;
		lseek(file_img, fatOffset, SEEK_SET);
		printf("fat offset is %x\n", fatOffset);
		unsigned int emptyClus = 0x0;
		int flag = write(file_img, &emptyClus, 4);
		clust = nextClust;
		nextClust = updateClust(clust);
	}

	//find next dir entry, if next dir in the curr clust
	if ((dirEntry_offset + 32) < curr_offset + BytsPerClus)
	{
		printf("Next entry in the same clust\n");
		lseek(file_img, dirEntry_offset + 32, SEEK_SET);
		struct DIRENTRY nextDir;
		read(file_img, &nextDir, sizeof(struct DIRENTRY));
		if (nextDir.DIR_Name[0] != 0)
		{
			printf("Deleted file is not the last entry\n");
			file.DIR_Name[0] = 0xe5;
		}
		else
		{
			printf("Deleted file is the last entry\n");
			file.DIR_Name[0] = 0x0;
		}
	}
	else
	{
		//find the clust that contains next dir entry
		unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + curr_clust * 4;
		lseek(file_img, fatOffset, SEEK_SET);
		unsigned int nextDir_clust;
		read(file_img, &nextDir_clust, 4);
		//get offset of next dir entry
		unsigned int nextDir_offset = BPB_BytsPerSec * (FirstDataSector + (nextDir_clust - 2) * BPB_SecPerClus);
		lseek(file_img, nextDir_offset, SEEK_SET);
		struct DIRENTRY nextDir;
		//read next dir entry
		read(file_img, &nextDir, sizeof(struct DIRENTRY));
		if (nextDir.DIR_Name[0] != 0)
		{
			//the deleted file is not the dir entry
			file.DIR_Name[0] = 0xe5;
		}
		else
		{
			file.DIR_Name[0] = 0x0;
		}
	}
	//write file back to dir entry
	lseek(file_img, dirEntry_offset, SEEK_SET);
	write(file_img, &file, sizeof(struct DIRENTRY));
}

void cpFile(tokenlist *tokens)
{
	if (tokens->size < 3)
	{
		printf("ERROR: Too few arguments. Usage： rm <file>\n");
		return;
	}

	struct DIRENTRY file = pathSearch(tokens->items[1]);

	if (file.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
		return;
	}
	else if (file.DIR_Attr == 0X10)
	{
		printf("ERROR: Cannot copy a directory\n");
		return;
	}
	unsigned int clust = file.DIR_FstClusHI << 16;
	clust += file.DIR_FstClusLO;
	struct openFile *temp = isFileOpened(clust, tokens->items[1]);
	if (temp != NULL)
	{
		printf("ERROR: %s is open. Please close it before cp\n", tokens->items[1]);
		return;
	}
	//check if the dest exists
	struct DIRENTRY dest = pathSearch(tokens->items[2]);
	int flag = 0;
	if (strcmp(tokens->items[1], tokens->items[2]) == 0)
	{
		flag = 0; //cp to the same file, does nothing
		return;
	}
	else if (dest.DIR_Attr == 0x10)
	{
		flag = 1; //cp to a directory
	}
	else if (dest.DIR_Name[0] != 0)
	{
		flag = 2; //overwrite the existing file
	}
	else if (dest.DIR_Name[0] == 0)
	{
		flag = 3; //cp to a new file
	}
	unsigned int sourceClust = curr_clust;
	printf("flag is %d\n", flag);
	//open source for reading
	openFile(tokens->items[1], "r");
	char *writeString = (char *)malloc(file.DIR_FileSize + 1);
	memset(writeString, '\0', file.DIR_FileSize);
	writeString = readFile(tokens->items[1], file.DIR_FileSize);
	//close source file
	closeFile(tokens->items[1]);

	if (flag == 1)
	{
		unsigned int destClust = dest.DIR_FstClusHI << 16;
		destClust += dest.DIR_FstClusLO;
		//cd to destClust
		curr_clust = destClust;
		create(tokens->items[1]);
		printf("create cp file\n");
		//open dest file and write
		openFile(tokens->items[1], "w");
		printf("open\n");
		writeFile(tokens->items[1], file.DIR_FileSize, writeString);
		closeFile(tokens->items[1]);
		//change working directory back to source
		curr_clust = sourceClust;
		return;
	}
	if (flag == 2)
	{
		rmFile(tokens->items[2]);
	}
	//create dest file and write
	create(tokens->items[2]);
	openFile(tokens->items[2], "w");
	writeFile(tokens->items[2], file.DIR_FileSize, writeString);
	closeFile(tokens->items[2]);
}

void mvFile(char *FILE, char *dest)
{
	//get starting offset of current working directory
	unsigned int curr_offset = BPB_BytsPerSec * (FirstDataSector + (curr_clust - 2) * BPB_SecPerClus);

	struct DIRENTRY src = pathSearch(FILE);
	//store starting offset of file dir entry
	unsigned int src_offset = lseek(file_img, 0, SEEK_CUR) - 32;
	//get dest file
	struct DIRENTRY destFile = pathSearch(dest);

	if (src.DIR_Name[0] == 0x0)
	{
		printf("ERROR: File not exits\n");
		return;
	}
	else if (destFile.DIR_Name[0] != 0 && destFile.DIR_Attr != 0x10)
	{
		printf("ERROR: The name is already being used by anohter file\n");
		return;
	}
	else if (src.DIR_Attr == 0x10)
	{
		printf("ERROR: Cannot move directory: invalid destination argument\n");
		return;
	}
	else if (destFile.DIR_Attr == 0x10)
	{
		//move to  directory
		unsigned int sourceClust = curr_clust;
		curr_clust = destFile.DIR_FstClusHI << 16;
		curr_clust += destFile.DIR_FstClusLO;
		create(FILE);
		struct DIRENTRY temp = pathSearch(FILE);
		//store start offset of dest direntry
		unsigned int dest_offset = lseek(file_img, 0, SEEK_CUR) - 32;
		temp = src;
		lseek(file_img, dest_offset, SEEK_SET);
		write(file_img, &temp, sizeof(struct DIRENTRY));
		//change back to src dir
		curr_offset = sourceClust;
		//remove src file entry
		int last = isLastEntry(src_offset, curr_offset);
		if (last == 1)
		{
			src.DIR_Name[0] = 0x0;
		}
		else
		{
			src.DIR_Name[0] = 0xe5;
		}
		lseek(file_img, src_offset, SEEK_SET);
		write(file_img, &src, 32);
		curr_clust = sourceClust;
	}
	else if (destFile.DIR_Name[0] == 0)
	{
		//update file name

		for (int i = 0; i < strlen(dest); i++)
		{
			src.DIR_Name[i] = toupper(dest[i]);
			printf("%c", toupper(dest[i]));
		}
		for (int i = strlen(dest); i < 11; i++)
		{
			src.DIR_Name[i] = ' ';
		}
		printf("file name after update is %s\n", src.DIR_Name);
		printf("src offset is %x\n", src_offset);
		lseek(file_img, src_offset, SEEK_SET);
		int flag = write(file_img, &src, 32);
		printf("write %d\n", flag);
	}
}

int isLastEntry(unsigned int src_offset, unsigned int curr_offset)
{
	//find next dir entry, if next dir in the curr clust
	if ((src_offset + 32) < curr_offset + BytsPerClus)
	{
		printf("Next entry in the same clust\n");
		lseek(file_img, src_offset + 32, SEEK_SET);
		struct DIRENTRY nextDir;
		read(file_img, &nextDir, sizeof(struct DIRENTRY));
		if (nextDir.DIR_Name[0] != 0)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		//find the clust that contains next dir entry
		unsigned int fatOffset = BPB_RsvdSecCnt * BPB_BytsPerSec + curr_clust * 4;
		lseek(file_img, fatOffset, SEEK_SET);
		unsigned int nextDir_clust;
		read(file_img, &nextDir_clust, 4);
		//get offset of next dir entry
		unsigned int nextDir_offset = BPB_BytsPerSec * (FirstDataSector + (nextDir_clust - 2) * BPB_SecPerClus);
		lseek(file_img, nextDir_offset, SEEK_SET);
		struct DIRENTRY nextDir;
		//read next dir entry
		read(file_img, &nextDir, sizeof(struct DIRENTRY));
		if (nextDir.DIR_Name[0] != 0)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
}