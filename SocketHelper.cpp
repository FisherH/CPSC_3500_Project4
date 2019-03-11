//Authors: Jack Arnold, Michelle simoni, fisher harris
//will read from the buffer and also insure that if recv doesnt return all the
//data that it waits to make sure all data has been recieved
// 
#include "SocketHelper.h"
#include <errno.h>
#include <cstring>


//creating the class
SocketHelper::SocketHelper()
{
    memset(&messageBuff, 0, sizeof(cbuf));
    read_socket = -1;
}

//reset function to reset the buffer
void SocketHelper::Reset()
{
    messageBuff.rpos = messageBuff.wpos = 0;
}


//will read the body length of the bites even if the message received
//by the server is short
int SocketHelper::ReadLine(char *dst, size_t size, int bodyLength)
{
    unsigned int i = 0;
    ssize_t n;
    while (i < size) {
        // no more data in buffer, read it from socket
        if (messageBuff.rpos == messageBuff.wpos) {
            size_t wpos = messageBuff.wpos % CBSIZE;
            size_t bytesToRead = (CBSIZE - wpos);

            // if we are reading the body make sure we don't ask for more
            // than the server is sending us
            if (bodyLength != -1)
            {
                int bytesLeft = bodyLength - i;
                if (bytesToRead > bytesLeft)
                    bytesToRead = bytesLeft;
            }

   
            if((n = recv(read_socket, messageBuff.buf + wpos, bytesToRead, 0)) < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            } else if (n == 0)
                return 0;
            messageBuff.wpos += n;
            
        }
        dst[i++] = messageBuff.buf[messageBuff.rpos++ % CBSIZE];
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
    
    return i;
}
