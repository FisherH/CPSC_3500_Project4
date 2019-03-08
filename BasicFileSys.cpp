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
    //Dafuq?
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
  bool pass = true;
  bool found = false;

  //Why 256?
  char buffer[256];

  dirblock_t directoryPointer;
  bfs.read_block(curr_dir, (void *) &directoryPointer);

  if(directoryPointer.num_entries > 0)
  {
    for(int i = 0; i <= directoryPointer.num_entries; i++)
    {
      if(strcmp(directoryPointer.dir_entries[i].name, name) == 0)
      {
        found = true;
        if(!isValidDirectory(directoryPointer.dir_entries[i].block_num))
        {
          pass = false;
          strcat(buffer, "500 File is not a direcotry\r\n");
        }
        else
        {
          strcat(buffer, "200 OK\r\n");
          curr_dir = directoryPointer.dir_entries[i].block_num;
        }
      }
    }
  }

  //If we reached and didnt find the directory
  if(found && pass)
    strcat(buffer, "503 Directory does not exist\r\n");

  cout << buffer << endl;
  send(fs_sock, buffer, sizeof(buffer), 0);
  
}

// switch to home directory
void FileSys::home() 
{
  char buffer[BLOCK_COUNT];
  curr_dir = 1;
  strcpy(buffer, "200 OK\r\n Length: 0\r\n\r\n");
  send(fs_sock, buffer, sizeof(buffer), 0);

}

// remove a directory
void FileSys::rmdir(const char *name)
{
  struct dirblock_t currentBlockPointer;
  char currentFileName[FILENAME_MAX_SIZE];
  char newFileName[FILENAME_MAX_SIZE];

  strcpy(currentFileName, name);
  bfs.read_block(currentFileName, (void*)&currentBlockPointer);

  bool pass = true;
  char buffer[1024];

  short foundBlock = 0;
  unsigned int foundIndex = 0;
  bool found = false;
  dirblock_t FoundDirectoryPointer;

  for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    strcpy(currentFileName, currentBlockPointer.dir_entries[i].name);
    if(strcmp(currentFileName, newFileName) == 0)
    {
      pass = true;
      found = true;
      foundIndex = i;
      foundBlock = currentBlockPointer.dir_entries[foundIndex].block_num;
      bfs.read_block(foundBlock, (void*)&FoundDirectoryPointer);
    }
    else
      pass = false;
  }

  if(found)
    pass = true;
  }

  if(!isValidDirectory(foundBlock))
  {
    if(pass)
    {
      pass = false;
      strcpy(buffer, "500: File is not a directory\r\n");
    }

    else if(!found && pass)
    {
      pass = false;
      strcpy(buffer, "503: File does not exist\r\n");
    }

    else if(FoundDirectoryPointer.num_entries != 0 && pass)
    {
      error = true;
      strcpy(buffer, "507: Directory is not empty\r\n");
    }

    else
    {
      if(found && !pass)
      {
        bfs.reclaim_block(currentBlockPointer.dir_entries[foundIndex].block_num);
        currentBlockPointer.dir_entries[foundIndex].name[0] = '\0';
        currentBlockPointer.dir_entries[foundIndex].block_num = 0;
        currentBlockPointer.num_entries --;

        bfs.write_block(curr_dir, (void*)&currentBlockPointer);
        strcpy(buffer, "200 OK\r\n Length: 0\r\n\r\n");
      }
    }
    send(fs_sock, buffer, buffer, sizeof(buffer), 0);
  }
}

// list the contents of current directory
void FileSys::ls()
{
  char bufferStart[] = "";
  string buffer = "";
  char msg[2048];
  char msgLength[80];
  dirblock_t currentBlockPointer;
  char currentFileName[FILENAME_MAX_SIZE];

  bfs.read_block(curr_dir, (void*)&currentBlockPointer);
  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    if(currentBlockPointer.dir_entries[i].block_num > 0)
    {
      if(isValidDirectory(currentBlockPointer.dir_entries[i].block_num))
      {
        string temp(currentBlockPointer.dir_entries[i].name)
        buffer += (temp + "/ ");
      }
      else
      {
        string temp(currentBlockPointer.dir_entries[i].name);
        buffer += (temp + " ")
      }
    }
  }

  stcpy(msg, bufferStart);
  springf(msgLength, "Length: %d", sizeof(buffer.c_str()));
  strcat(msg, msgLength);
  strcat(msg, "\r\n\r\n");
  strcat(msg, buffer.c_str());
  send(fs_sock, msg, sizeof(msg), 0);
}

// create an empty data file
void FileSys::create(const char *name)
{
  char buffer[1024];
  bool pass = true;

  dirblock_t currentBlockPointer;
  char newFileName[FILENAME_MAX_SIZE + 1];
  char currentFileName[FILENAME_MAX_SIZE + 1];
  strcpy(newFileName, name);

  bfs.read_block(curr_dir, (void*)&currentBlockPointer);
  if(pass)
  {
    for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++)
    {
      strcpy(currentFileName, currentBlockPointer.dir_entries[i].name);
      if(strcmp(currentFileName, newFileName) == 0)
      {
        strcpy(buffer, "502: File exits\r\n");
        error = true;
      }
    }
  }

  if(pass)
  {
    if(strlen(name) > FILENAME_MAX_SIZE + 1)
    {
      srtcpy(buffer, "504: File name is too large\r\n");
      pass = false;
    }
  }

  //Create new free block
  short block_num = bfs.get_free_block();
  if(block_num == 0)
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

  if(pass)
  {
    if(currentBlockPointer.num_entries >= MAX_DIR_ENTRIES)
    {
      strcpy(buffer, "506: Directory is full\r\n");
    }
  }

  if(pass)
  {
    inode_t currentDirectoryInode;
    currentDirectoryInode.magic = INODE_MAGIC_NUM;
    currentDirectoryInode.size = 0;
    for(int k = 0; k < MAX_DIR_ENTRIES; k++)
    {
      currentDirectoryInode.blocks[k] = 0;
    }
    bfs.write_block(block_num, (void*) &currentDirectoryInode);
    strcpy(buffer, "200 OK\r\n Length: 0\r\n");
    strcat(buffer, newFileName);
    strcpy(currentBlockPointer.dir_entries[currentBlockPointer.num_entries].name, newFileName);

    currentBlockPointer.dir_entries[currentBlockPointer.num_entries].block_num = block_num;

    bfs.write_block(curr_dir, (void *)&currentBlockPointer);

  }
  send(fs_sock, buffer, sizeof(buffer), 0);
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
  bool found = false;
  bool pass = true;

  //why 256?
  char buffer[FILENAME_MAX_SIZE + 256];

  datablock_t* fileContents = new datablock_t;
  dirblock_t dirBlock;
  bfs.read_block(curr_dir, (void *) &dirBlock);
  for(int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    if(strcmp(name, dirBlock.dir_entries[i].name == 0)
    {
      found = true;
      if(isValidDirectory(dirBlock.dir_entries[i].block_num))
      {
        strcpy(buffer, "501: File is a directory\r\n");
        pass = false;
        delete fileContents;
        break;
      }
      else if(!isValidDirectory(dirBlock.dir_entries[i].block_num))
      {
        inode_t fileInode;
        bfs.read_block(dirBlock.dir_entries[i].block_num, (void *) &fileInode);

        string fileSizeStr = to_string(fileInode.size);
        int byte_count = (fileSizeStr.length() + 1);
        char * fileSize = new char[byte_count];
        strcpy(fileSize, fileSizeStr.c_str());

        strcat(buffer, "200 OK\r\n Length:");
        strcat(buffer, fileSize);
        strcat(buffer, "\r\n");
        //Why a second time
        strcat(buffer, "\r\n");

        delete[] fileSize;

        for(int j = 0; j < sizeof(fileInode.blocks); j++)
        {
          bfs.read_block(fileInode.blocks[j], (void *) &fileContents);
          strcat(buffer, fileContents->data);
        }

        delete fileContents;
        //Send buffer to socket here **************************************************************************** ??
      }
    }
  }

  if(!found && pass)
  {
    strcpy(buffer, "503: File does not exist\r\n");
  }

  delete file_contents;
  send(fs_sock, buffer, sizeoof(buffer), 0);
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
  bool found = false;
  bool pass = true;

  //Why + 256?
  char buffer [FILENAME_MAX_SIZE + 256];

  datablock_t* fileContents = new datablock_t;
  dirblock_t dir_block;

  bfs.read_block(curr_dir, (void *) &dirBlock);

  for(int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    if(strcmp(name, dir_block.dir_entries[i].block_num))
    {
      strcpy(buffer, "501: File is a directory\r\n");
      pass = false;
      delete fileContents;
    }

    else if (!isValidDirectory(dirBlock.dir_enries[i].block_num))
    {
      inode_t fileInode;
      unsigned int bytesLeft;

      bfs.read_block(dirBlock.dir_entries[i].block_num, (void *) &fileInode);

      //Doubble equal?
      if(n > fileInode.size)
        bytesLeft = fileInode.size;

      string sizeStr = to_string(fileInode.size);
      int byteCount = (sizeStr.length() + 1);
      char* fileSize = new char[16];
      strcpy(fileSize, sizeStr.c_str());

      strcat(buffer, "200 OK\r\n Length:");
      strcat(buffer, fileSize);
      strcat(buffer, "\r\n");
      strcat(buffer, "\r\n");

      delete[] fileSize;

      for(int j = 0; j < sizeof(fileInode.blocks);j++)
      {
        bfs.read_block(fileInode.blocks[j], (void *) &fileContents);
        if(bytesleft < BLOCK_SIZE)
          strncat(buffer, file_contents->data, bytes_left);
        else
        {
          strcat(buffer, fileContents->data);
          bytesLeft -= BLOCK_SIZE;
        }
      }
    }
  }

  if(!found && pass)
    strcpy(buffer, "503: File does not exist\r\n");

  delete fileContents;
  send(fs_sock, buffer, sizeof(buffer), 0);
  
}

// delete a data file
void FileSys::rm(const char *name)
{
  char buffer[FILENAME_MAX_SIZE + 256];
  dirblock_t currBlock;
  bool pass = true;
  bool found = false;

  bfs.read_block(curr_dir,(void*)& curr_block);

  for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    if(strcmp(currBlock.dir_entries[i].name, name) == 0)
    {
      if(!isValidDirectory(currBlock.dir_entries[i].block_num))
      {
        found = true;
        inode_t inodeBlock;
        bfs.read_block(currBlock.dir_entries[i].block_num, (void*)&inodeBlock);

        for(size_t j = 0; j < (inodeBlock.size / BLOCK_SIZE) + 1; j++)
        {
          bfs.reclaim_block(inodeBlock.blocks[i]);
          inodeBlock.blocks[i] = 0;
          inode_block.size = 0;
        }
        bfs.reclaim_block(currBlock.dir_entries[i].block_num);

        currBlock.dir_entries[i].name[0] = '\0';
        currBlock.dir_entries[i].block_num = 0;

        currBlock.num_entries--;
        bfs.write_block(curr_dir,(void*)&curr_block);
      }
      else
      {
        strcpy(buffer, "501: File is a directory\r\n");
        pass = false;
      }
    }
  }

  if(!found && pass)
  {
    strcpy(buffer, "503: File does not exist\r\n");
    pass = false;
  }
  else
  {
    strcpy(buffer, "200: OK\r\n Length: 0\r\n\r\n")
  }

  send(fs_sock, buffer, sizeof(buffer), 0);
}
// display stats about file or directory
void FileSys::stat(const char *name)
{
  dirblock_t currentBlockPointer;
  bfs.read_block(curr_dir, (void *)&currentBlockPointer);
  bool found = false;
  int foundIndex;

  char newFileName[FILENAME_MAX_SIZE + 1];
  char currentFileName[FILENAME_MAX_SIZE + 1];
  strcpy(fileName, name);

  dirblock_t FoundDirectoryPointer;
  bool dirCheck;
  char bufferStart[] = "200: OK\r\n";
  char msgLength[80];
  char message[2048];
  char buffer[2048];

  bool checkFirstBlock = false;
  int count = 0;
  short firstBlock;
  char directoryBlock[BLOCK_COUNT];
  char inodeBlock[BLOCK_COUNT];
  char bytesInFile[BLOCK_COUNT];
  char numberOfBlocks[BLOCK_COUNT];
  char firstBlockNum[BLOCK_COUNT];

  for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++)
  {
    strcpy(currentFileName, currentBlockPointer.dir_entries[i].name);
    if(strcpy(currentFileName, newFileName) == 0)
    {
      found = true;
      foundIndex = i;
      foundIndex = includebfs.read_block(currentBlockPointer.dir_entries[i].block_num, (void*)&FoundDirectoryPointer);
    }
  }

  if(found)
  {
    if(isValidDirectory(currentBlockPointer.dir_entries[foundIndex].block_num))
    {
      strcat(buffer, "\nDirectory name: ");
      strcat(buffer, file_name);
      strcat(buffer, "\nDirectory block: ");
      sprintf(directoryBlock, "%d", currentBlockPointer.dir_entries[foundIndex].block_num);
      strcat(buffer, directoryBlock);
    }
    else
    {
      inode_t newInode;
      bfs.read_block(currentBlockPointer.dir_entries[foundIndex].block_num, (void*) &newInode);

      for(int k = 0; k < BLOCK_COUNT; k++)
      {
        if(newInode.blocks[k] != 0) 
        {
          count++;

          if(!checkFirstBlock)
          {
            firstBlock = k;
            checkFirstBlock = true;
          }
        }
      }

      strcat(buffer, "\nInode block: ");
          sprintf(inode_block, "%d", curr_block_ptr.dir_entries[found_index].block_num);
          strcat(buffer, inode_block);
          strcat(buffer, "\nBytes in file: ");
          sprintf(bytes_in_file, "%d", new_inode.size);
          strcat(buffer, bytes_in_file);
          strcat(buffer, "\nNumber of blocks: ");
          sprintf(number_of_blocks, "%d", counter);
          strcat(buffer, number_of_blocks);
          strcat(buffer, "\nFirst block: ");
          sprintf(first_block_num, "%d", first_block);
          strcat(buffer, first_block_num);
    }
      
      strcpy(message, bufferStart);
      sprintf(msgLength, "Length: %d \r\n", sizeof(buffer));
      strcat(message, msgLength);
      //strcat(message, buffer);
      strcat(buffer, message);
    
  }
  else
    strcpy(message, "503 File does not exist\r\n");
  
  send(fs_sock, buffer, sizeof(buffer), 0);
}

// HELPER FUNCTIONS (optional)
const bool FileSys::isValidDirectory(short block_num)
{
  dirblock_t target_dir;
  bfs.read_block(block_num, (void *) &target_dir);

  return (target_dir == NULL);
}
