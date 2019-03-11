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
#include "SocketHelper.h"

using namespace std;

//constant variables

const int BACKLOG = 5;
const int BUFFER_SIZE = 2048;

void error(char* message)
{
  perror(message);
  exit(0);
}

int main(int argc, char* argv[]) {

  //variables
  int sockfd, newsockfd, port,clilen;
  char buffer[BUFFER_SIZE];

  //socket addresses
  struct sockaddr_in server_ad, clientAd;
  struct sockaddr_storage their_addr;

  //address info
  socklen_t sin_size;
  if (argc <2) {
    cout << "Usage: ./nfsserver port#\n";
    return -1;
  }

  //create socket and accept client connection
  port = atoi(argv[1]);
  cout<<"Connecting to port..."<< port<<endl;
  sockfd = socket(AF_INET,SOCK_STREAM, 0);

  if(sockfd<0) {
    perror("ERROR opening socket");
    exit(1);
  }
  
  //now will bind the socket to a port number
  bzero((char*) &server_ad, sizeof(server_ad));
  server_ad.sin_family=AF_INET;
  server_ad.sin_addr.s_addr = INADDR_ANY;
  server_ad.sin_port= htons(port);
  memset(server_ad.sin_zero, '\0', sizeof server_ad.sin_zero);

  int bindCheck= ::bind(sockfd,(struct sockaddr *) &server_ad, sizeof(struct sockaddr));
  if(bindCheck == -1)
    cout<<"Could not bind socket.\n";

  //waiting for the clien to make a connection
  if(:: listen(sockfd, BACKLOG)<0)
    error((char*)"ERROR on listen");

  //accept a request from client
  newsockfd= ::accept(sockfd,(struct sockaddr*) &clientAd, (socklen_t*) &clilen);
  if(newsockfd <0)
    error((char*)"ERROR on accept");

  //mount the file system
  printf("Client connecting...\n");
  FileSys fs;
  fs.mount(newsockfd);

  // use helper class to read lines from the client
  SocketHelper socketHelper;
  socketHelper.read_socket = newsockfd;

  int n;
  while(1) {
    bzero(buffer, BUFFER_SIZE);
    socketHelper.Reset();
    n=socketHelper.ReadLine(buffer, sizeof(buffer));
    fs.execute_command(buffer);

    if(n==0)
    {
      perror("Connection closed.");
      break;
    }
  }

  //close the sockets
  close(newsockfd);
  close(sockfd);

  //unmout file system
  fs.unmount();
  return 0;
}



