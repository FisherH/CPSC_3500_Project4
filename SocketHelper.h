//Authors: Jack Arnold, Fisher Harris, Michelle Smoni
//description: this class is a helper object for reading from the socket
//this is to ensure the full message is recieved instead of 
//sending a partial message

#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define CBSIZE 2048

struct cbuf {
    char buf[CBSIZE]; //creating the buffer
    unsigned int rpos, wpos;  // read and write position in the buffer
} ;


class SocketHelper
{
public:
	//creating the socket helper buffer
    SocketHelper();

    //reading command
    int ReadLine(char *line, size_t maxLength, int bodyLength=-1);
    
    //will reset the rpos and wpos to zero
    void Reset();


    int read_socket;
private:
	//cbuf object
    cbuf messageBuff;
};
