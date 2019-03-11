#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"
#include <string>
using namespace std;

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

// mounts the file system
void FileSys::mount(int sock) 
{
	bfs.mount();
  	curr_dir = 1; // by default current directory is home directory, in disk block #1
  	fs_sock = sock;
}

// unmounts the file system
void FileSys::unmount() 
{
  	bfs.unmount();
  	close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name)
{
  	char buffer[1024];
	bool pass = true;

  	dirblock_t curr_block_ptr;
  	char file_name[MAX_FNAME_SIZE + 1];
  	char curr_file_name[MAX_FNAME_SIZE + 1];
  	strcpy(file_name, name);

  	if(pass && (strlen(name) > MAX_FNAME_SIZE +1))
  	{
    	//504
    	strcpy(buffer, "200: Filename too long\r\nLength: 0\r\n\r\n");
    	pass = false;
  	}

	bfs.read_block(curr_dir, (void*)&curr_block_ptr);
	if(pass)
  	for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++) {
    	strcpy(curr_file_name, curr_block_ptr.dir_entries[i].name);
    	if (strcmp(curr_file_name, file_name) == 0){
        //502
      	strcpy(buffer, "200: Directory exists\r\nLength: 0\r\n\r\n");
        cout << "Operation failed: Directory exists." << endl;
      	pass = false;
        break;
    	}
    }
  
  	// make new free block
  	short block_num = bfs.get_free_block();
  	if (block_num == 0){
    	block_num = bfs.get_free_block();
    	if (block_num == 0){
      		if(pass){
        		//505
        		strcpy(buffer, "202: Disk is full\r\nLength: 0\r\n\r\n");
        		cout << "Operation failed: Disk is full." << endl;
        		pass = false;
      		}
    	}
 	}

  	if (pass && (curr_block_ptr.num_entries == MAX_DIR_ENTRIES)){
    	//505
    	strcpy(buffer, "506: Directory is full\r\nLength: 0\r\n\r\n");
    	cout << "Operation failed: Directory is full." << endl;
    	pass = false;
  	}
  

  //fill new directory block_num to hold 0 to show blocks are unused
  if(pass) {
    dirblock_t new_block;
    new_block.magic = DIR_MAGIC_NUM;
    new_block.num_entries = 0;

    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
      new_block.dir_entries[i].block_num = 0;
      new_block.dir_entries[i].name[0] = '\0';
    }

    bfs.write_block(block_num, (void*)&new_block);

    strcpy(buffer, "200: Success\r\nLength: 0\r\n\r\n");

    //strcat(buffer, file_name);
    strcpy(curr_block_ptr.dir_entries[curr_block_ptr.num_entries].name, file_name);
    curr_block_ptr.dir_entries[curr_block_ptr.num_entries].block_num = block_num;
    curr_block_ptr.num_entries++;
    bfs.write_block(curr_dir, (void*)&curr_block_ptr);
  }
  send(fs_sock, buffer, strlen(buffer), 0);
}

// switch to a directory
void FileSys::cd(const char *name)
{
	bool error = false;
  	bool found = false;
  	char buffer[256];

  	buffer[0] = '\0';
  	//retrieve current directory data block
  	dirblock_t dir_ptr;
  	bfs.read_block(curr_dir, (void *) &dir_ptr);

  	printf("num entries %d\n", dir_ptr.num_entries);
  	//check if any sub directories exist in current directory
  	if(dir_ptr.num_entries > 0){
    	//check each sub directory and check directory names for match
    	for(int i= 0; i < dir_ptr.num_entries; i++){
      		if(strcmp(dir_ptr.dir_entries[i].name, name) == 0){
        		found = true;
        		if(!is_directory(dir_ptr.dir_entries[i].block_num)){
          			error = true;
                //500
          			strcat(buffer, "500: target is a file, not a directory\r\n");
                break;
        		}
        		else
            {
          			strcat(buffer, "200: OK\r\n");
          			curr_dir = dir_ptr.dir_entries[i].block_num;
        		}
      		}
    	}
  	}
  
  	// if this point reached, no matching directory found
 	if(!found && !error)
    strcat(buffer, "503:File does not exist\r\n");
  	
  //cout << buffer << endl;
  strcat(buffer, "Length: 0\r\n\r\n");
  send(fs_sock, buffer, strlen(buffer), 0);
}

// switch to home directory
void FileSys::home(){
	char buffer[1024];
  curr_dir = 1;
  strcpy(buffer, "200:Success\r\n");
  strcat(buffer, "Length: 0\r\n\r\n");
  send(fs_sock, buffer, strlen(buffer), 0);
}

// remove a directory
void FileSys::rmdir(const char *name)
{
  	struct dirblock_t curr_block_ptr;
 	  char file_name[MAX_FNAME_SIZE + 1];
  	char curr_file_name[MAX_FNAME_SIZE + 1];
  	strcpy(file_name, name);
  	bfs.read_block(curr_dir, (void*)&curr_block_ptr);

  	bool error = false;
  	char buffer[1024] = {0};
  	short found_block = 0;
  	unsigned int found_index = 0;
  	bool found = false;
  	dirblock_t found_dir_ptr;
  	//finds the directory to remove wthin the current directory, sets found
  	//bool to true if it exists in the directory
  	for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    	strcpy(curr_file_name, curr_block_ptr.dir_entries[i].name);
    	
    	if (strcmp(curr_file_name, file_name) == 0)
    	{
      	error = false;
      	found = true;
      	found_index = i;
      	found_block = curr_block_ptr.dir_entries[found_index].block_num;
      	bfs.read_block(found_block, (void*)&found_dir_ptr);
    	}
    	else
      		error = true;
  	}

  	if(found)
    	error = false;
  
  	//ensures the found directory is a directory and not a file
  	if(!is_directory(found_block)){
    	if(!error){
      	error = true;
      	strcpy(buffer, "500: File is not a directory\r\nLength: 0\r\n\r\n");
    	}
  	}
  	
  	//if the found bool was not set to true then the filename passed in does
  	//not exist and the error message is wrote into the buffer
  	else if(!found && !error){
    	error = true;
    	strcpy(buffer ,"503: File does not exsit\r\nLength: 0\r\n\r\n");
  	}

  	//if the found directory is a directory but still contains files it cannot
  	//be removed and this error is wrote into the buffer
  	else if(found_dir_ptr.num_entries != 0 && !error){
    	error = true;
    	strcpy(buffer, "507: Directory is not empty\r\nLength: 0\r\n\r\n");
  	}

  	else
  	{
    	//if error is still set to false then remove the directory and set all
    	//values back to null/0 as well as reclaiming its block num
    	if(found && !error){
     		bfs.reclaim_block(curr_block_ptr.dir_entries[found_index].block_num);
      	curr_block_ptr.dir_entries[found_index].name[0] = '\0';
      	curr_block_ptr.dir_entries[found_index].block_num = 0;
      	curr_block_ptr.num_entries--;
      	bfs.write_block(curr_dir, (void*)&curr_block_ptr);
      	strcpy(buffer, "200: OK\r\n Length: 0\r\n\r\n");
    	}
  	}
  	send(fs_sock, buffer, strlen(buffer), 0);
}

// list the contents of current directory
void FileSys::ls()
{
	char bufferStart[] = "200: OK\r\n";
  string buffer = "";

  dirblock_t curr_block_ptr;
  char curr_file_name[MAX_FNAME_SIZE + 1];
  //read the data held at the current directory from the disk
  bfs.read_block(curr_dir, (void*)&curr_block_ptr);

  int dirCount = 0;

  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
  	if(curr_block_ptr.dir_entries[i].block_num > 0){

      //For directories
      if(is_directory(curr_block_ptr.dir_entries[i].block_num)){
        dirCount++;
        string temp(curr_block_ptr.dir_entries[i].name);
        buffer += temp;
        buffer += "/ ";
      }

      //For files
      else{
        string temp(curr_block_ptr.dir_entries[i].name);
        buffer += temp;
        buffer += " ";
      }
    }
  }

  if(dirCount == 0)
    buffer += "empty directory";

  char msg[2048];
  char msgLength[80];

	strcpy(msg, "200: OK\r\n");
	sprintf(msgLength, "Length: %d", strlen(buffer.c_str()));
	strcat(msg, msgLength);
	strcat(msg, "\r\n\r\n");
	strcat(msg, buffer.c_str());
	send(fs_sock, msg, strlen(msg), 0);
}

// create an empty data file
void FileSys::create(const char *name)
{
	char buffer[1024] = {0};
  	bool pass = true;

  	dirblock_t curr_block_ptr;
  	char file_name[MAX_FNAME_SIZE + 1];
  	char curr_file_name[MAX_FNAME_SIZE + 1];
  	strcpy(file_name, name);

  	bfs.read_block(curr_dir, (void*)&curr_block_ptr);
  	if(pass)
    	for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    		strcpy(curr_file_name, curr_block_ptr.dir_entries[i].name);
    		if(strcmp(curr_file_name, file_name) == 0){
      		strcpy(buffer, "502: File exists\r\nLength: 0\r\n\r\n"); 
      		pass = false;
    		}
    	}

    	if(pass && (strlen(name) > MAX_FNAME_SIZE + 1)){
    		strcpy(buffer, "504: File name is too long\r\nLength: 0\r\n\r\n");
    		pass = false;
    	}
  	

  	// make new free block
  	short block_num = bfs.get_free_block();
  	if(block_num == 0){
    	block_num = bfs.get_free_block();
    	if(block_num == 0){
    		if(pass){
      		strcpy(buffer, "505: Disk is full\r\nLength: 0\r\n\r\n");
      		pass = false;
    		}
    	}
  	}

	if(pass && (curr_block_ptr.num_entries == MAX_DIR_ENTRIES)){
		strcpy(buffer, "506: Directory is full\r\nLength: 0\r\n\r\n");
		pass = false;
	}

  	// fill new inode block_num to hold 0 to show blocks are unused
  	if(pass) {
    	inode_t curr_dir_inode;
    	curr_dir_inode.magic = INODE_MAGIC_NUM;
    	curr_dir_inode.size = 0;
    
    	for(int k = 0; k < MAX_DIR_ENTRIES; k++)
      		curr_dir_inode.blocks[k] = 0;
    
    	bfs.write_block(block_num, (void*) &curr_dir_inode);
    	strcpy(buffer, "200: OK\r\nLength: 0\r\n\r\n");
    	strcat(buffer,file_name);
    	strcpy(curr_block_ptr.dir_entries[curr_block_ptr.num_entries].name, file_name);
    	curr_block_ptr.dir_entries[curr_block_ptr.num_entries].block_num = block_num;
    	curr_block_ptr.num_entries++; // add this file to parent directory
    	bfs.write_block(curr_dir, (void*)&curr_block_ptr);
  	}
  	send(fs_sock, buffer, strlen(buffer), 0);
}

// append data to a data file
void FileSys::append(const char *name, const char *data)
{
	bool error = false;
  	bool found = false;

	// may need to increase buffer size to account for terminal messages
  	char buffer [1024] = {0};
  	inode_t fileInode;
  	int fileLength;
  	datablock_t* fileContents = new datablock_t;
  	dirblock_t dirBlock;
  	bfs.read_block(curr_dir, (void *) &dirBlock);

  	for(int i = 0; i < MAX_DIR_ENTRIES; i++)
    {
      //target file found in current element in dir_entries
      	if(strcmp(name, dirBlock.dir_entries[i].name)==0)
        {

          	found = true;
          	if(is_directory(dirBlock.dir_entries[i].block_num)) {
              strcpy(buffer, "501: File is a directory\r\nLength: 0\r\n\r\n");
              error = true;
              break;
            }

          	else if(!is_directory(dirBlock.dir_entries[i].block_num)) {
              	int appendLength = strlen(data);
              	int inodeBlock = dirBlock.dir_entries[i].block_num;
              	bfs.read_block(inodeBlock, (void *) &fileInode);
              	// current size of the file
              	fileLength = fileInode.size;
              	// make sure we don't exceed max file size

        	    if ((fileLength + appendLength) > MAX_FILE_SIZE) {
                  error = true;
                  strcpy(buffer, "508: Append exceeds maximum file size\r\nLength: 0\r\n\r\n");
                  break;
                }

              	// copy data into file
              	int ByteCountDown = appendLength;
              	int lastUsedIndex = fileLength / BLOCK_SIZE;  // jump to last block of data in the file
             	
             	while (ByteCountDown)
                {

                  	// if no data block assigned then allocate one
                  	if (fileInode.blocks[lastUsedIndex] == 0)
                    {

                      // allocate another block for this file
                      short blockNum = bfs.get_free_block();
                      //will check if the block is still zero after requesting a free block
                      //will then request again to make sure not error

                      if (blockNum == 0){
                        blockNum = bfs.get_free_block();
                        //checking to see if it is still one then we know the disk is full 
                        if (blockNum == 0){
                          	if(!error){
                           	 	strcpy(buffer, "505: Disk is full\r\nLength: 0\r\n\r\n");
                            	error = true;
                            	break;

                          }
                        }
                      }

                    // add this block fo the inode for this file
                    fileInode.blocks[lastUsedIndex] = blockNum;
                }

                // read the block and then append to it
                bfs.read_block(fileInode.blocks[lastUsedIndex], (void *) fileContents);
                int bytesToCopy, offsetInData, bytesFreeInBlock;
                char *eof_ptr;

                // the offset into this data block where eof is and where we will write
                offsetInData = fileLength % BLOCK_SIZE;

                // The number of bytes that are free to be written to in this block
                bytesFreeInBlock = BLOCK_SIZE - offsetInData;

                // get the pointer to append data to
                eof_ptr = fileContents -> data + offsetInData;

                // calculate how much to copy (can't be more that whats left in this block or the block size)
                bytesToCopy = ByteCountDown;

                if (bytesToCopy > bytesFreeInBlock)
                	bytesToCopy = bytesFreeInBlock;

                // append the data to a block 
                memcpy(eof_ptr, data, bytesToCopy);
                data += bytesToCopy;
                ByteCountDown -= bytesToCopy;
                fileLength += bytesToCopy;
                // write out the modified block
                bfs.write_block(fileInode.blocks[lastUsedIndex], (void *) fileContents);
                // move on to the next block for this file
                lastUsedIndex++;
                }


            // we have appended all data, now update the inode with new file size and blocks we possibly created

            fileInode.size = fileLength;
            bfs.write_block(inodeBlock, (void *) &fileInode);
            
            // send buffer to socket here
            strcpy(buffer, "200 OK\r\nLength: 0\r\n\r\n");
            break;
            }
        }
    }

    // if point reached, file not found.
    // ERROR 503 File does not exist
  	if(!found && !error)
    	strcpy(buffer, "503: File does not exist\r\nLength: 0\r\n\r\n");
  
  	if (fileContents != NULL)
    	delete fileContents;

    send(fs_sock, buffer, strlen(buffer), 0);
}

// delete a data file
void FileSys::rm(const char *name)
{
	bool pass = true;
  	bool found = false;

  	char buffer[MAX_FNAME_SIZE+256];
 	dirblock_t curr_block;
  	bfs.read_block(curr_dir,(void*)& curr_block);

  for (unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
  	if (strcmp(curr_block.dir_entries[i].name, name) == 0){
  		if (!is_directory(curr_block.dir_entries[i].block_num)){
        found = true;
        inode_t inode_block;
        bfs.read_block(curr_block.dir_entries[i].block_num, (void*)&inode_block);
        for (size_t j = 0; j < (inode_block.size / BLOCK_SIZE) + 1; j++){
          			bfs.reclaim_block(inode_block.blocks[i]);
          			inode_block.blocks[i] = 0;
          			inode_block.size = 0;
        }
        bfs.reclaim_block(curr_block.dir_entries[i].block_num);

        curr_block.dir_entries[i].name[0] = '\0';
        curr_block.dir_entries[i].block_num = 0;
        curr_block.num_entries--;
       	bfs.write_block(curr_dir,(void*)& curr_block);

      	}

      	else {
        	strcpy(buffer, "501: File is a directory\r\nLength: 0\r\n\r\n");
        	pass = false;
      	}
    	}
  	}
  	if(!found && pass){
    	strcpy(buffer, "503: File does not exist\r\nLength: 0\r\n\r\n");
    	pass = false;
  	}

  	else
    	strcpy(buffer, "200: OK\r\nLength: 0\r\n\r\n");
  	
  	//send buffer to client
  	send(fs_sock, buffer, strlen(buffer), 0);
}

// display stats about file or directory
void FileSys::stat(const char *name)
{
  	bool found = false;
  	int found_index;

	dirblock_t curr_block_ptr;
  	bfs.read_block(curr_dir, (void*)&curr_block_ptr);

  	char file_name[MAX_FNAME_SIZE+1];
  	char curr_file_name[MAX_FNAME_SIZE+1];
  	strcpy(file_name, name);

  	dirblock_t found_dir_ptr;
  	bool dir_check;
  	char bufferStart[] = "200: OK\r\n";
  	char msgLength[80];
  	char message[2048];
  	char buffer[2048];

  	//arrays to hold inode stats and holders for inode values
  	bool check_first_block = false;
  	int counter = 0;
  	short first_block;
  	char directory_block[MAX_DATA_BLOCKS];
  	char inode_block[MAX_DATA_BLOCKS];
  	char bytes_in_file[MAX_FILE_SIZE];
  	char number_of_blocks[MAX_DATA_BLOCKS];
  	char first_block_num[MAX_DATA_BLOCKS];

  	memset(buffer, 0, sizeof(buffer));  // bugbug zero this out

  	for(unsigned int i = 0; i < MAX_DIR_ENTRIES; i++){
    	strcpy(curr_file_name, curr_block_ptr.dir_entries[i].name);
    	if(strcmp(curr_file_name, file_name) == 0){
      		found = true;
      		found_index = i;
      		bfs.read_block(curr_block_ptr.dir_entries[i].block_num, (void*)&found_dir_ptr);
    	}
  	}

  	if(found){
    	if(is_directory(curr_block_ptr.dir_entries[found_index].block_num)){
      		strcat(buffer, "\nDirectory name: ");
      		strcat(buffer, file_name);
      		strcat(buffer, "\nDirectory block: ");
      		sprintf(directory_block, "%d", curr_block_ptr.dir_entries[found_index].block_num);
      		strcat(buffer, directory_block);
    	}
    	else{
      		inode_t new_inode;
      		bfs.read_block(curr_block_ptr.dir_entries[found_index].block_num,
                     (void*)&new_inode);
      		for(int k = 0; k < MAX_DATA_BLOCKS; k++){
        		if(new_inode.blocks[k] != 0){
          			counter = counter + 1;
          			// determine first block
          			if(!check_first_block){
            			first_block = k;
            			check_first_block = true;
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
    	sprintf(msgLength, "Length: %ld \r\n\r\n", strlen(buffer)); // bugbug changed to strlen
    	strcat(message, msgLength);
    	strcat(message, buffer);
    }

	else
   		strcpy(message, "503: File does not exist\r\nLength: 0\r\n\r\n");
  
  	send(fs_sock, message, strlen(message), 0);  // bugbug send this
}

// Executes the command. Returns true for quit and false otherwise.
bool FileSys::execute_command(string command_str)
{
  // parse the command line
  cout << "New command issued, parsing.." << endl;
  struct FileSys::Command command = parse_command(command_str);

 	// copy the contents of the string to char array
  const char* command_name = command.name.c_str();
  const char* file_name = command.file_name.c_str();
  const char* append_data = command.append_data.c_str();

  cout << "New command: " << command.name.c_str() << endl;
  //printf("command: '%s'\n", command_name);

  // look for the matching command
  if (command.name == "") 
    return false;
  
  else if (command.name == "mkdir") 
   	mkdir(file_name);
	  
	else if (command.name == "cd") 
    cd(file_name);
	  
	else if (command.name == "home") 
	 home();
	  
	else if (command.name == "rmdir") 
	 rmdir(file_name);
	  
	else if (command.name == "ls") 
	 ls();
	  
	else if (command.name == "create") 
    create(file_name);
	  
	else if (command.name == "append") 
    append(file_name, append_data);
	  
	else if (command.name == "cat") 
  	cat(file_name);
  
	else if (command.name == "head") 
	{
		errno = 0;
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) 
    	head(file_name, n);

    else {
    	cerr << "Invalid command line: " << command.append_data;
    	cerr << " is not a valid number of bytes" << endl;
    	return false;
    }
  }

	else if (command.name == "rm")
    rm(file_name);
	  
	else if (command.name == "stat")
    stat(file_name);
	  
	else if (command.name == "quit")
		return true;
	  
	return false;
}

// display the contents of a data file
void FileSys::cat(const char *name)
{
	 //similar to append in the sense that you have to write data out to something 

  //in this case though we are reading from file and outputing to client 

  //so first have to find file then we would read the blocks from the file

  bool error = false;

  bool found = false;


  // may need to increase buffer size to account for terminal messages

  char buffer [MAX_FILE_SIZE + 256] = {0};


  datablock_t* fileContents = new datablock_t;

  dirblock_t dirBlock;

  bfs.read_block(curr_dir, (void *) &dirBlock);

  for(int i = 0; i < MAX_DIR_ENTRIES; i++)

    {

      //target file found in current element in dir_entries

      if(strcmp(name, dirBlock.dir_entries[i].name)==0)

        {

          found = true;

          if(is_directory(dirBlock.dir_entries[i].block_num))

            {

              strcpy(buffer, "501: File is a directory\r\nLength: 0\r\n\r\n");

              error = true;

              break;

            }

          else if(!is_directory(dirBlock.dir_entries[i].block_num))

            {

              inode_t fileInode;

              bfs.read_block(dirBlock.dir_entries[i].block_num, (void *) &fileInode);


              int fileLength = fileInode.size;

              string fileStrSize = to_string(fileInode.size);

              int byteCount = (fileStrSize.length() + 1);

              char* fileSize = new char[byteCount];

              strcpy(fileSize, fileStrSize.c_str());


              strcat(buffer, "200 OK\r\n Length:");

              strcat(buffer, fileSize);

              strcat(buffer, "\r\n");

              strcat(buffer, "\r\n");

              delete[] fileSize;


              int bytesLeft = fileLength;

              for(int j = 0; j < ARRAY_LENGTH(fileInode.blocks) && bytesLeft; j++)

                {

                  bfs.read_block(fileInode.blocks[j], (void *) fileContents);

                  if(bytesLeft < BLOCK_SIZE)

                    {

                      strncat(buffer, fileContents->data, bytesLeft);

                      bytesLeft = 0;

                    }

                  else

                    {

                      strncat(buffer, fileContents->data, BLOCK_SIZE);

                      bytesLeft -= BLOCK_SIZE;

                    }

                }


              break;

              // send buffer to socket here

            }

        }

    }


  // if point reached, file not found.

  // ERROR 503 File does not exist

  //this will delete the blocks we made to read data as it isnt necessary for 

  //reading as there is no file to read from 

  if(!found && !error)

    {

      strcpy(buffer, "503: File does not exist\r\nLength: 0\r\n\r\n");

    }


  if (fileContents != NULL)

    delete fileContents;

  send(fs_sock, buffer, strlen(buffer), 0);
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n)
{
  	bool found = false;
  bool error = false;
  char buffer [MAX_FILE_SIZE + 256] = {0};

  datablock_t* file_contents = new datablock_t;
  dirblock_t dir_block;

  // read contents of current directory's directory node into dir_ptr
  bfs.read_block(curr_dir, (void *) &dir_block);

  // look through all elements of dir_entries held in current directory blocks
  for(int i = 0; i < MAX_DIR_ENTRIES; i++)
    {
      //target file found in current element in dir_entries
      if(strcmp(name,dir_block.dir_entries[i].name)==0)
        {
          found = true;
          if(is_directory(dir_block.dir_entries[i].block_num))
            {
              strcpy(buffer, "501: File is a directory\r\nLength: 0\r\n\r\n");
              error = true;
              break;
            }
          else if(!is_directory(dir_block.dir_entries[i].block_num))
            {
              inode_t file_inode;
              unsigned int bytes_left;
              //read inode data for targeted file
              bfs.read_block(dir_block.dir_entries[i].block_num, (void *) &file_inode);

              bytes_left = n; 
              if(n > file_inode.size)
              {
                bytes_left = file_inode.size;
              }

              string size_str = to_string(bytes_left);
              int byte_count = (size_str.length() + 1);
              char* file_size = new char[16];
              strcpy(file_size, size_str.c_str());

              strcat(buffer, "200 OK\r\n Length:");
              strcat(buffer, file_size);
              strcat(buffer, "\r\n");
              strcat(buffer, "\r\n");
              delete[] file_size;

              //append each data block pointed to by inode_t.blocks[] to our buffer
              for(int j = 0; j < ARRAY_LENGTH(file_inode.blocks) && bytes_left; j++)
                {
                  bfs.read_block(file_inode.blocks[j], (void *) file_contents);
                  if(bytes_left < BLOCK_SIZE)
                  {
                    strncat(buffer, file_contents->data, bytes_left);
                    bytes_left = 0;
                  }
                  else
                  {
                    strncat(buffer, file_contents->data, BLOCK_SIZE);
                    bytes_left -= BLOCK_SIZE;
                  }
                }

                break;
            }
        }
    }
  // if point reached, file not found.
  // ERROR 503 File does not exist
  if(!found && !error)
  {
    strcpy(buffer, "503: File does not exist\r\nLength: 0\r\n\r\n");
  }
  if (file_contents != NULL)
    delete file_contents;
  send(fs_sock, buffer, strlen(buffer), 0);
}

// HELPER FUNCTIONS
const bool FileSys::is_directory(short block_num)
{
	//create dirblock_t to read block into
  	dirblock_t target_dir;
  	bfs.read_block(block_num, (void *) &target_dir);

  	if (target_dir.magic == DIR_MAGIC_NUM)
    	return true;
  
  	else 
    	return false;
}

// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
FileSys::Command FileSys::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct FileSys::Command empty = {"", "", ""};

  // grab each of the tokens (if they exist)
  struct Command command;
  istringstream ss(command_str);
  int num_tokens = 0;
  if (ss >> command.name) {
    num_tokens++;
    if (ss >> command.file_name) {
      num_tokens++;
      if (ss >> command.append_data) {
        num_tokens++;
        string junk;
        if (ss >> junk) {
          num_tokens++;
        }
      }
    }
  }

  // Check for empty command line
  if (num_tokens == 0) {
    return empty;
  }

  // Check for invalid command lines
  if (command.name == "ls" || command.name == "home" ||
      command.name == "quit")
  {
    if (num_tokens != 1) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "mkdir" ||
      command.name == "cd"    ||
      command.name == "rmdir" ||
      command.name == "create"||
      command.name == "cat"   ||
      command.name == "rm"    ||
      command.name == "stat")
  {
    if (num_tokens != 2) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "append" || command.name == "head")
  {
    if (num_tokens != 3) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else {
    cerr << "Invalid command line: " << command.name;
    cerr << " is not a command" << endl;
    return empty;
  }

  return command;
}
