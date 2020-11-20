# COP4610-Proj3
This project implement FAT32 file system </br>
## Build
`make` </br>
## Usage
`./project3 fat32.img`

## Functions 
### exit
by Liting Zhang </br>
Usage `exit`</br>
Implemented within main function. </br>

### info
by Liting Zhang </br>
Print the BPB information. </br>
Usage `info` </br>

### size 
by Liting Zhang </br>
Print the size of the given file. </br>
Usage `size <file>` </br>

### ls
by Liting Zhang </br>
list the file in the given directory </br>
Usage `ls <dir>` </br>

### cd  
by Liting Zhang </br>
Change the working directory to the given directory. </br>
Usage `cd <directory>` </br>
If `<directory>` is not specify, change working directory to root. </br>
Not able to change back to parent directory by using `..` </br>

### create
by Liting Zhang </br>
Create an empty file in the working directory. </br>
Usage `create <file>` </br>

### mkdir 
by Liting Zhang </br>
Create new directory. </br>
Usage `mkdir <directory>` </br>

### open
by Liting Zhang </br>
Open a file. </br>
Usage `open <file> <mode>`. Valid modes are `r, w, rw, wr`. </br>

### close
by Liting Zhang </br>
Close an opended file. Can only close a file A if the current working directory contains file A.</br>
Usage `close <file>`. 

### lseek
by Liting Zhang
Set offset in bytes for further reading or writing given an opened file. </br>
Usage `lseek <file> <offset>`
