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
void create(tokenlist *tokens);			//create file
void mkdir(tokenlist *tokens);
void openFile(tokenlist *tokens); //open file
//add to open file list
void addOpenFile(unsigned int clust, tokenlist *tokens);
struct openFile *isFileOpened(unsigned int clust, char *name); // if file is open
void freeFile(void);										   //free opened files
void closeFile(tokenlist *tokens);
void removeOpenFile(unsigned int clust, tokenlist *tokens);
void printlist();				   //print open file list
void lseekFile(tokenlist *tokens); //lseek an open file
void readFile(tokenlist *tokens);
void writeFile(tokenlist *tokens);
unsigned int updateClust(unsigned int clust);
void writeToFile(char *string, unsigned int start, unsigned int size);

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
int curr_clust;	 //current cluster
int pare_clust;	 //paraent cluster

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
			printf("%d\n", size(tokens));
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
			create(tokens);
		}
		else if (strcmp(command, "mkdir") == 0)
		{
			mkdir(tokens);
		}
		else if (strcmp(command, "open") == 0)
		{
			openFile(tokens);
		}
		else if (strcmp(command, "close") == 0)
		{
			closeFile(tokens);
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
			readFile(tokens);
		}
		else if (strcmp(command, "write") == 0)
		{
			writeFile(tokens);
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

	while (tempDir.DIR_Name[0] != 0x0 && Offset < 0xFFFFFF8)
	{

		if (tempDir.DIR_Attr != 0xF && tempDir.DIR_Name[0] != 0x0)
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
		//sector per clus is 1, so byts per sec = byts per clus
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
		if (Offset > nOffset + BPB_BytsPerSec)
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

void create(tokenlist *tokens)
{
	if (tokens->size < 2)
	{
		printf("ERROR: Too few arguments. Usage create <file>\n");
	}
	else
	{
		struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
		if (tempDir.DIR_Name[0] != 0x0)
		{
			printf("ERROR: File already exists\n");
		}
		else
		{
			struct DIRENTRY newFile;
			unsigned int emptClus = findEmptyClus();

			unsigned int lastClus = 0x0FFFFF8;
			lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + emptClus * 4, SEEK_SET);
			//set emptclus to last cluster
			write(file_img, &lastClus, 4);
			//copy name
			char *fileName = tokens->items[1];
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
			unsigned int offSet;
			offSet = BPB_BytsPerSec * (FirstDataSector + (curr_clust - 2) * BPB_SecPerClus);

			while (1)
			{

				lseek(file_img, offSet, SEEK_SET);
				read(file_img, &temp, 32);
				if (temp.DIR_Name[0] == 0x0)
				{
					lseek(file_img, offSet, SEEK_SET);
					int a = write(file_img, &newFile, 32);
					break;
				}
				offSet += 32;
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
			unsigned int offSet;
			offSet = BPB_BytsPerSec * (FirstDataSector + (curr_clust - 2) * BPB_SecPerClus);

			while (1)
			{

				lseek(file_img, offSet, SEEK_SET);
				read(file_img, &temp, 32);
				if (temp.DIR_Name[0] == 0x0)
				{
					lseek(file_img, offSet, SEEK_SET);
					int a = write(file_img, &newDir, 32);
					break;
				}
				offSet += 32;
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
			offSet = BPB_BytsPerSec * (FirstDataSector + (emptClus - 2) * BPB_SecPerClus);
			lseek(file_img, offSet, SEEK_SET);
			write(file_img, &pareDir, 32);
			//add current dir entry
			strncpy(temp.DIR_Name, ".           ", 11);
			write(file_img, &temp, 32);
			//add empty dir entry
			struct DIRENTRY emptEntry;
			emptEntry.DIR_Name[0] = 0x0;
			write(file_img, &emptEntry, 32);
		}
	}
}

void openFile(tokenlist *tokens)
{ //r. w. rw. wr
	if (tokens->size < 3)
	{
		printf("ERROR: Too few arguments. Usage open <file> <mode>\n");
	}
	else
	{
		struct DIRENTRY file = pathSearch(tokens->items[1]);
		if (file.DIR_Name[0] == 0x0)
		{
			printf("ERROR: File not exits\n");
		}
		else if (file.DIR_Attr & 0x10)
		{

			printf("ERROR: %s is a direcotry. Cannot open a directory\n", tokens->items[1]);
		}
		else if (strcmp(tokens->items[2], "rw") != 0 && strcmp(tokens->items[2], "w") != 0 &&
				 strcmp(tokens->items[2], "wr") != 0 && strcmp(tokens->items[2], "r") != 0)
		{
			printf("ERROR: Invalid mode. Valid modes are r, w, rw, wr\n");
		}
		else if ((strcmp(tokens->items[2], "rw") == 0 | strcmp(tokens->items[2], "w") == 0 |
				  strcmp(tokens->items[2], "wr") == 0) &&
				 (file.DIR_Attr & 0x01))
		{
			printf("ERROR: Ths file is read only\n");
		}
		else
		{
			unsigned int clust = file.DIR_FstClusHI << 16;
			clust += file.DIR_FstClusLO;
			unsigned int offSet = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
			if (isFileOpened(clust, tokens->items[1]) != NULL)
			{
				printf("ERROR: Fill already open\n");
			}
			else
			{
				addOpenFile(clust, tokens);
				printf("Open %s in %s mode\n", tokens->items[1], tokens->items[2]);
			}
		}
	}
}

void addOpenFile(unsigned int clust, tokenlist *tokens)
{
	struct openFile *file_ptr = (struct openFile *)malloc(sizeof(struct openFile));
	file_ptr->firstClus = clust;
	file_ptr->offSet = 0;
	strcpy(file_ptr->name, tokens->items[1]);
	strcpy(file_ptr->mode, tokens->items[2]);
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

void closeFile(tokenlist *tokens)
{
	if (tokens->size < 2)
	{
		printf("ERROR: Too few arguments. Usage: close <File>");
	}
	else
	{
		struct DIRENTRY file = pathSearch(tokens->items[1]);
		if (file.DIR_Name[0] == 0x0)
		{
			printf("ERROR: File not exits\n");
		}
		else if (file.DIR_Attr & 0x10)
		{

			printf("ERROR: %s is a direcotry. Cannot open a directory\n", tokens->items[1]);
		}
		else
		{
			unsigned int clust = file.DIR_FstClusHI << 16;
			clust += file.DIR_FstClusLO;
			if (isFileOpened(clust, tokens->items[1]) == NULL)
			{
				printf("ERROR: Fill is not open\n");
			}
			else
			{
				removeOpenFile(clust, tokens);
			}
		}
	}
}

void removeOpenFile(unsigned int clust, tokenlist *tokens)
{
	struct openFile *current = head;
	struct openFile *previous = NULL;
	if (head == NULL)
	{
		return;
	}
	while (current->firstClus != clust | strcmp(current->name, tokens->items[1]) != 0)
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
		printf("ERROR: Too few arguments. Usage: lseek <file> <size>");
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

void readFile(tokenlist *tokens)
{
	if (tokens->size < 3)
	{
		printf("ERROR: Too few arguments. Usage: read <file> <size>");
	}
	else
	{
		struct DIRENTRY file = pathSearch(tokens->items[1]);
		if (file.DIR_Name[0] == 0x0)
		{
			printf("ERROR: File not exits\n");
		}
		else if (file.DIR_Attr == 0X10)
		{
			printf("ERROR: Cannot read a directory\n");
		}
		else
		{
			unsigned int clust = file.DIR_FstClusHI << 16;
			clust += file.DIR_FstClusLO;
			struct openFile *temp = isFileOpened(clust, tokens->items[1]);
			if (isFileOpened(clust, tokens->items[1]) == NULL)
			{
				printf("ERROR: File is not open. Please open the file first\n");
			}
			else if (strcmp(temp->mode, "w") == 0)
			{
				printf("ERROR: This file is open in write mode only\n");
			}
			else
			{
				unsigned int readBytes = strtoul(tokens->items[2], NULL, 10);
				unsigned int byteToRead = readBytes;
				unsigned int tempOffset = temp->offSet;

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
					printf("%s", data);
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
						printf("%s", data);
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

						printf("%s", data);
						byteToRead = 0;
					}
				}
				printf("\n");
			}
		}
	}
}

void writeFile(tokenlist *tokens)
{
	if (tokens->size < 4)
	{
		printf("ERROR: Too few arguments. Usage: write <file> <size> <\"STRING\">\n");
		return;
	}
	else
	{
		struct DIRENTRY file = pathSearch(tokens->items[1]);
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
			struct openFile *temp = isFileOpened(clust, tokens->items[1]);
			if (isFileOpened(clust, tokens->items[1]) == NULL)
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

			unsigned int writeBytes = strtoul(tokens->items[2], NULL, 10);
			unsigned int byteToWrite = writeBytes;
			//get write string
			unsigned char string[writeBytes + 3];
			memset(string, '\0', writeBytes + 3);
			for (int i = 3; i < tokens->size - 1; i++)
			{
				strcat(string, tokens->items[i]);
				strcat(string, " ");
			}
			strcat(string, tokens->items[tokens->size - 1]);
			printf("write string is %s\n; ", string);
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
					unsigned char tempString[byteToWrite + 1];
					memset(tempString, '\0', byteToWrite + 1);
					strncpy(tempString, string + 1, byteToWrite);
					int temp = write(file_img, &tempString, byteToWrite);
					printf("write %d bytes; %s\n", temp, tempString);
					byteToWrite -= byteToWrite;
					printf("pointer is at %x\n", offset + tempOffset);
				}
				else
				{
					//write on current clust first
					writeToFile(string, 0, BytsPerClus - tempOffset);
					byteToWrite -= (BytsPerClus - tempOffset);
					printf("pointer is at %x\n", offset + tempOffset);
					while (byteToWrite > 0)
					{	//Allocate new clust
						//link next clus
						//lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4, SEEK_SET);
						//unsigned int test;
						//read(file_img, &test, 4);
						//printf("before update %d\n", test);
						unsigned int nextclust;
						nextclust = updateClust(clust);
						printf("next clust is %d\n", nextclust);
						//if last clust, allocate new clust
						if(nextclust >= 0xFFFF08)
						{
							nextclust = findEmptyClus();
							lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + clust * 4, SEEK_SET);
							write(file_img, &nextclust, 4);
							printf("allocate new class %d\n", nextclust);
						}

						//printf("empty clust is %d; write \n", nextclust);
						clust = nextclust;//move to next clust
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
					unsigned int lastClus = 0x0FFFFF8;
					lseek(file_img, BPB_RsvdSecCnt * BPB_BytsPerSec + clust* 4, SEEK_SET);
					//set last cluster
					write(file_img, &lastClus, 4);
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
					unsigned char tempString[BytsPerClus - tempOffset + 1];
					memset(tempString, '\0', BytsPerClus - tempOffset + 1);
					strncpy(tempString, string + 1, BytsPerClus - tempOffset);
					int temp = write(file_img, &tempString, (BytsPerClus - tempOffset));
					printf("write %d bytes; %s\n", temp, tempString);
					byteToWrite -= (BytsPerClus - tempOffset);

					clust = updateClust(clust);
					offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
					lseek(file_img, offset, SEEK_SET);
					printf("pointer is at %x\n", offset + tempOffset);
				}
				else
				{
					//write on current clust first
					unsigned char tempString[byteToWrite + 1];
					memset(tempString, '\0', byteToWrite + 1);
					strncpy(tempString, string + 1, byteToWrite);
					int temp = write(file_img, &tempString, byteToWrite);
					printf("write %d bytes; %s\n", temp, tempString);
					byteToWrite -= byteToWrite;
				}

				while (byteToWrite > 0)
				{
					unsigned int startByte = writeBytes - byteToWrite;
					printf("start byt is %d\n", startByte);
					if (byteToWrite >= BytsPerClus)
					{
						unsigned char tempString[BytsPerClus + 1];
						memset(tempString, '\0', BytsPerClus + 1);
						strncpy(tempString, string + startByte + 1, BytsPerClus);

						int temp = write(file_img, &tempString, BytsPerClus);
						byteToWrite -= BytsPerClus;

						clust = updateClust(clust);
						offset = BPB_BytsPerSec * (FirstDataSector + (clust - 2) * BPB_SecPerClus);
						lseek(file_img, offset, SEEK_SET);
						printf("pointer is at %x\n", offset + tempOffset);
					}
					else
					{
						unsigned char tempString[byteToWrite + 1];
						memset(tempString, '\0', byteToWrite + 1);
						strncpy(tempString, string + startByte + 1, byteToWrite);
						int temp = write(file_img, &tempString, byteToWrite);
						printf("write %d bytes; %s\n", temp, tempString);
						byteToWrite -= byteToWrite;
						printf("pointer is at %x\n", offset + tempOffset);
					}
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
	strncpy(tempString, string + start + 1, size);
	int temp = write(file_img, &tempString, size);
	printf("write %d bytes; %s\n", temp, tempString);
}