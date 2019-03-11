#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define CBSIZE 2048

typedef struct cbuf {
    char buf[CBSIZE];
    unsigned int rpos, wpos;  // read and write position in the buffer
} cbuf_t;


class SocketHelper
{
public:
    SocketHelper();
    int ReadLine(char *line, size_t maxLength, int bodyLength=-1);
    void Reset();

    int read_socket;
private:
    cbuf_t cbuf;
};