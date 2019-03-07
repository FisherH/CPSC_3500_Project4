// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"
#include <sys/socket.h>

using namespace std;

//Will be moved to FileSys.h eventually
//Here for convinience currently. 
int FILENAME_MAX_SIZE = 30;
int BLOCK_SIZE = 128;
int BLOCK_COUNT = 1024;
int MAX_DIR_ENTRIES = 10000;

// mounts the file system
void FileSys::mount(int sock) {
  bfs.mount();
  curr_dir = 1; //by default current directory is home directory, in disk block #1
  fs_sock = sock; //use this socket to receive file system operations from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name)
{
	char buffer[BLOCK_COUNT];
	bool pass = true;

	//Finish implementation
	dirblock_t currentBlockPointer;
	char currentFileName[FILENAME_MAX_SIZE];
	char newFileName[FILENAME_MAX_SIZE];
	
	//copy the new name into the new file name array
	strcpy(newFileName, name);

	bfs.read_block(curr_dir, (void*)&curr_block_ptr);

	//Check if file path already exists
	if(pass)
	{
		for(long i = 0; i < MAX_DIR_ENTRIES; i++)
		{
			strcpy(currentFileName, currentBlockPointer.dir_entries[i].name);
			if(strcmp(currentFileName, newFileName) == 0
			{
				strcpy(buffer, "502: File exists\r\n");
				pass = false;
			}
		}
	}

	//Check if file name is too long
	if(pass)
	{
		if(strlen(name) > FILENAME_MAX_SIZE + 1) 
		{
			strcpy(buffer, "504: File name is too long\r\n");
			pass = false;
		}
	}

	//Create a new freeblock
	short block_num = bfs.get_free_block();
	if (block_num == 0)
	{
		block_num = bfs.get_free_block();
		if(block_num == 0)
		{
			if(pass)
			{
				strcpy(buffer, "505: Disk is full\r\n");
				pass = false;
			}
		}
	}

	//check if the directory is full
	if(pass)
	{
		if(currentBlockPointer.num_entries == MAX_DIR_ENTRIES)
		{
			strcpy(buffer, "506: Directory is full\r\n");
			pass = false;
		}
	}

	if(pass)
	{
		dirblock_t newBlock;
		//newBlock.magic = DIR_MAGIC_NUM
		newBlock.num_entries = 0;
		for (int i = 0; i < MAX_DIR_ENTRIES; i++)
		{
			newBlock.dir_entries[i].block_num = 0;
			newBlock.dir_entries[i].name[0] = '\0';
		}
		bfs.write_block(block_num, (void*)&currentBlockPointer);
	}
	send(fs_sock, buffer, sizeof(buffer), 0);

}

// switch to a directory
void FileSys::cd(const char *name)
{
}

// switch to home directory
void FileSys::home() 
{

}

// remove a directory
void FileSys::rmdir(const char *name)
{
}

// list the contents of current directory
void FileSys::ls()
{
}

// create an empty data file
void FileSys::create(const char *name)
{
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
}

// delete a data file
void FileSys::rm(const char *name)
{
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
}

// HELPER FUNCTIONS (optional)

