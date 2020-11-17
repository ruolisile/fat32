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
void cd(tokenlist *tokens);//cd 
unsigned int findEmptyClus(void);//find next empty clus
void create(tokenlist * tokens); //create file
void mkdir(tokenlist *tokens);
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
int pare_clust;	 //paraent cluster


struct DIR{
	unsigned int pare_clust;
};
struct DIR curr_dir;
struct DIR pare_dir;

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
		else if(strcmp(command, "cd") == 0)
		{
			cd(tokens);
		}
		else if(strcmp(command, "create") == 0)
		{
			create(tokens);
		}
		else if(strcmp(command, "mkdir") == 0)
		{
			mkdir(tokens);
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
			if(tempDir.DIR_Name[0] == 0x0)
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
				printf("%s", tokens->items[1]);
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

	while (tempDir.DIR_Name[0] != 0x0 && Offset <  0xFFFFFF8)
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

		if(Offset > nOffset + BPB_BytsPerSec)
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
		if(Offset > nOffset + BPB_BytsPerSec)
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
//only work for directory with no duplicate names
void cd(tokenlist *tokens)
{
	if(tokens->size == 1)
	{
		curr_clust = BPB_RootClus;//cd to root
		pare_clust = BPB_RootClus;

	}
	else if(strcmp(tokens->items[1], ".") == 0)
	{
		curr_clust = curr_clust;
		pare_clust = pare_clust;
	}
	else if(strcmp(tokens->items[1], "..") == 0)
	{
		if(curr_clust == BPB_RootClus)
		{
			printf("ERROR: Already in root directory\n");
		}
		
	}
	else
	{
		struct DIRENTRY tempDir = pathSearch(tokens->items[1]);
		if(tempDir.DIR_Name[0] == 0x0)
		{
			printf("ERROR: Directory not found\n");
		}
		else if(tempDir.DIR_Attr != 0x10)//not a dir
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
		if(tempDir.DIR_Name[0] != 0x0)
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
			for(int i = 0; i < strlen(fileName); i++)
			{
				newFile.DIR_Name[i] = toupper(fileName[i]);
			}
			for(int i = strlen(fileName); i < 11; i++)
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
				if(temp.DIR_Name[0] == 0x0)
				{
					lseek(file_img, offSet, SEEK_SET);
					int a = write(file_img, &newFile, 32);
					return;
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
		if(tempDir.DIR_Name[0] != 0x0 && tempDir.DIR_Attr == 0x10)
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
			for(int i = 0; i < strlen(fileName); i++)
			{
				newDir.DIR_Name[i] = toupper(fileName[i]);
			}
			for(int i = strlen(fileName); i < 11; i++)
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
				if(temp.DIR_Name[0] == 0x0)
				{
					lseek(file_img, offSet, SEEK_SET);
					int a = write(file_img, &newDir, 32);
					break;
				}
				offSet += 32;
			}	

			struct DIRENTRY pareDir;
			strncpy(pareDir.DIR_Name, "..           ", 11);
			newDir.DIR_Attr = 0x10;
			newDir.DIR_NTRes = 0;
			newDir.DIR_CrtTimeTenth = 0;
			newDir.DIR_CrtTime = 0;
			newDir.DIR_CrtDate = 0;
			newDir.DIR_LstAccDate = 0;
			newDir.DIR_FstClusHI = (curr_clust >> 16) & 0xFFFF;
			newDir.DIR_WrtTime = 0;
			newDir.DIR_WrtDate = 0;
			newDir.DIR_FstClusLO = 0xFFFF & curr_clust;
			newDir.DIR_FileSize = 0;

			offSet =  BPB_BytsPerSec * (FirstDataSector + (emptClus - 2) * BPB_SecPerClus);
			lseek(file_img, offSet, SEEK_SET);
			write(file_img, &pareDir, 32);
			printf("Wrote parent to offset %d", offSet);

			strncpy(temp.DIR_Name, ".           ", 11);
			write(file_img, &temp, 32);
			
			struct DIRENTRY emptEntry;
			emptEntry.DIR_Name[0] = 0x0;
			write(file_img, &emptEntry, 32);
		}
		
	}	
}