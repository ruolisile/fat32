# COP4610-Proj3
This project implements a FAT32 file system. </br>
Code authors are properly credited within their appropriate functions. </br>

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
Prints the BPB information. </br>
Usage `info` </br>

### size 
by Liting Zhang </br>
Prints the size of the given file. </br>
Usage `size <file>` </br>

### ls
by Liting Zhang and Amanda Lovett</br>
Lists the files in the given directory </br>
Usage `ls <dir>` </br>

### cd  
by Liting Zhang and Amanda Lovett</br>
Changes the working directory to the given directory. </br>
Usage `cd <directory>` </br>
If `<directory>` is not specify, change working directory to root. </br>

### create
by Liting Zhang </br>
Creates an empty file in the working directory. </br>
Usage `create <file>` </br>

### mkdir 
by Liting Zhang </br>
Creates a new directory. </br>
Usage `mkdir <directory>` </br>

### mv
By Liting Zhang and Amanda Lovett</br>
Usage `mv <from> <to>`. Move a file named `<from>` to directory `<to>` or change the file name `<from>` to `<to>`</br>
mv for file into directory may be buggy</br>

### open
by Liting Zhang </br>
Opens a file. </br>
Usage `open <file> <mode>`. Valid modes are `r, w, rw, wr`. </br>

### close
by Liting Zhang </br>
Closes an opened file. Can only close a file A if the current working directory contains file A.</br>
Usage `close <file>`. 

### lseek
by Liting Zhang </br>
Set offset in bytes for further reading or writing given an opened file. </br>
Usage `lseek <file> <offset>`

### read
by Liting Zhang </br>
Reads a file given number of bytes to read. </br>
Usage `read <file> <size>`

### write
by Liting Zhang </br>
Writes text to an open file given text and size of the text.</br>
Usage `write <file> <size> <"string">`. Text to be written into a file should be inside qutation mark. 

### rm
by Liting Zhang</br>
Usage `mv <file>`.</br>
Deletes the file named `file` from the current working directory. </br>

### cp
by Liting Zhang </br>
Usage `cp <file> <to>`. </br>
Copies `file` to directory `to` or file `to`. File `to` will be overwrriten or created.</br>

### rmdir
by Amanda Lovett </br>
Useage `rmdir <directory>`. </br>
Removes an empty directory `directory`. </br>

## Known Errors and Bugs
Once enter the file system, double quotation marks must be in pairs or it will fall into segmentation fault. </br> 
Bug testing and code review by Amanda Lovett and Liting Zhang.</br>
