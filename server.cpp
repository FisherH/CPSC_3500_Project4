#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "FileSys.h"
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

using namespace std;

//constant variables

const int BACKLOG=5;
const int BUFFER_SIZE=2048;

void error(char* message)
{
    perror(message);
    exit(0);
}


int main(int argc, char* argv[]) {
    
    //variables
    int sockfd, newsockfd, port,clilen;
    //const the buffer
    char buffer[BUFFER_SIZE];
    
    //creating socket addresses
    struct sockaddr_in server_ad, clientAd;
    struct sockaddr_storage their_addr;
    
    //the connecting address info
    socklen_t sin_size;
    if (argc < 2) {
        cout << "Usage: ./nfsserver port#\n";
        return -1;
    }
    port = atoi(argv[1]);
    cout<<"Connecting to port"<< port<<endl;
    
    //networking part: create the socket and accept the client connection
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
        error((char*)"ERROR opening socket");
    
    //now will bind the socket to a port number
    bzero((char*) &server_ad, sizeof(server_ad));
    server_ad.sin_family=AF_INET;
    server_ad.sin_addr.s_addr = INADDR_ANY;
    server_ad.sin_port= htons(port);
    if(:: bind(sockfd,(struct sockaddr *) &server_ad, siezof(server_ad))<0)
        error((char*)"ERROR on bind");
    
    //int sock; //change this line when necessary!
    
    //waiting for the clien to make a connection
    if(:: listen(sockfd, BACKLOG)<0)
        error((char*)"ERROR on listen");
    
    //accept a request from client
    newsockfd= ::accept(sockfd,(struct sockaddr*) &clientAd, (socklen_t*) &clilen);
    if(newsockfd <0)
        error((char*)"ERROR on accept");
    
    
    //mount the file system
    FileSys fs;
    fs.mount(sock); //assume that sock is the new socket created
    //for a TCP connection between the client and the server.
    
    
    //loop: get the command from the client and invoke the file
    //system operation which returns the results or error messages back to the clinet
    //until the client closes the TCP connection.
    
    int n;
    while(1) {
        bzero(buffer, BUFFER_SIZE);
        n= recv(newsockfd, buffer, sizeof(buffer), 0 );
        
        fs.execute_command(buffer);
        
        if(n==0)
        {
            perror("connection closed");
            break;
        }
    }
    
    //closing the socket
    close(newsockfd);
    
    
    //close the listening socket
    close(sockfd);
    
    //unmout the file system
    fs.unmount();
    
    return 0;
}

