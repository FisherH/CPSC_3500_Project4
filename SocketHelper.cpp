// SocketHelper 
//
// Reads \n terminated lines from a socket while using an internal buffer
// to handle the cases where recv does not return all of the data in a line
// or returns data that straddles a line feed.
// 
#include "SocketHelper.h"
#include <errno.h>
#include <cstring>

SocketHelper::SocketHelper()
{
    memset(&cbuf, 0, sizeof(cbuf));
    read_socket = -1;
}

void SocketHelper::Reset()
{
    cbuf.rpos = cbuf.wpos = 0;
}

// Read a linee of text from the socket that is terminated by line feed.
// we use an internal buffer since we recv can return any number of bytes
// less than what we request.
// When bodyLength is -1 (default) we read until a line feed is found.  
// However if bodyLength is specified then  
// we will simply read bodyLength number of bytes from the socket regardless
// of contents.  This is used to read the body of a server response
int SocketHelper::ReadLine(char *dst, size_t size, int bodyLength)
{
    unsigned int i = 0;
    ssize_t n;
    while (i < size) {
        // no more data in buffer, read it from socket
        if (cbuf.rpos == cbuf.wpos) {
            size_t wpos = cbuf.wpos % CBSIZE;
            size_t bytesToRead = (CBSIZE - wpos);

            // if we are reading the body make sure we don't ask for more
            // than the server is sending us
            if (bodyLength != -1)
            {
                int bytesLeft = bodyLength - i;
                if (bytesToRead > bytesLeft)
                    bytesToRead = bytesLeft;
            }

            //printf("recv ask %ld\n", bytesToRead);
            if((n = recv(read_socket, cbuf.buf + wpos, bytesToRead, 0)) < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            } else if (n == 0)
                return 0;
            cbuf.wpos += n;
            //printf("recv returned %ld bytes\n", n);
        }
        dst[i++] = cbuf.buf[cbuf.rpos++ % CBSIZE];
        // bail out when line terminated unless we are reading body
        if (dst[i - 1] == '\n' && bodyLength == -1)
            break;
        
        // once we have read all the bytes requested bail out
        if (bodyLength != -1 && i == bodyLength)
            break;
    }
    if(i == size) {
         fprintf(stderr, "line too large: %d %ld\n", i, size);
         return -1;
    }

    dst[i] = 0;
    //printf("Readline returning %d bytes\n", i);
    return i;
}