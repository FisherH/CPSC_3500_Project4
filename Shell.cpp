// CPSC 3500: Shell
//AUTHORS: Michelle Simoni, Fisher Harris, Jack Arnold
// Implements a basic shell (command line interface) for the file system

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring> 
using namespace std;

#include "Shell.h"


//////////////////consts////////////////////
static const string PROMPT_STRING = "NFS> ";	// shell prompt
const string END_TOKEN= "\r\n";

// Mount the network file system with server name and port number in the format of server:port
void Shell::mountNFS(string fs_loc) {
	//create the socket cs_sock and connect it to the server and port specified in fs_loc
	//if all the above operations are completed successfully, set is_mounted to true
    
    struct sockaddr_in server;
    vector<string> fileSysAddr;
    size_t pos=0, foundLoc;

    if ((foundLoc=fs_loc.find_first_of(':',pos)) != string::npos)
    {
        fileSysAddr.push_back(fs_loc.substr(pos,foundLoc-pos));
        pos = foundLoc+1;
    }
    
    fileSysAddr.push_back(fs_loc.substr(pos));
    
    //creating a socket
    cs_sock = socket(AF_INET,SOCK_STREAM,0);
    if(cs_sock <0)
    {
      perror("error in creating the socket");
        exit(0);
    }
    
    // Convert domain name to IP address
    struct hostent *server_ip;
    char ip[32];
    server_ip = gethostbyname(fileSysAddr[0].c_str());
    
    if (server_ip == NULL) 
    {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
    }
    inet_ntop(AF_INET, server_ip->h_addr_list[0], ip, sizeof(ip));

    printf("connecting to '%s' ip address: '%s' port: '%d'\n", fileSysAddr[0].c_str(), 
        ip,
        stoi(fileSysAddr[1].c_str()));

    //construct server address
    server.sin_addr.s_addr= inet_addr(ip); 
    server.sin_family = AF_INET;
    server.sin_port= htons(stoi(fileSysAddr[1].c_str()));
    
    //connect to the remote server if not will throw an error
    if(connect(cs_sock,(struct sockaddr *) &server, sizeof(server)) <0)
    {
        perror("error connection failed");
        exit(0);
    }
    
    // give this socket handle to the read helper class
    socketHelper.read_socket = cs_sock;

    is_mounted= true;
}

// Unmount the network file system if it was mounted
void Shell::unmountNFS() {
	// close the socket if it was mounted
  if(is_mounted)
  {
    close(cs_sock);
    is_mounted=false;
  }
}

// Remote procedure call on mkdir
void Shell::mkdir_rpc(string dname) {
  string line= "mkdir " + dname +END_TOKEN;   
  remote_procedure(line,"mkdir");

}

// Remote procedure call on cd
void Shell::cd_rpc(string dname) {
  string line= "cd "+ dname+END_TOKEN; 
  remote_procedure(line,"cd");

}

// Remote procedure call on home
void Shell::home_rpc() {
  string line="home"+END_TOKEN;
  remote_procedure(line,"home");

}

// Remote procedure call on rmdir
void Shell::rmdir_rpc(string dname) {
    string line="rmdir " + dname + END_TOKEN;  
    remote_procedure(line,"rmdir");
}

// Remote procedure call on ls
void Shell::ls_rpc() {
    string  line ="ls"+END_TOKEN;
    remote_procedure(line,"ls");
}

// Remote procedure call on create
void Shell::create_rpc(string fname) {
    string line = "create " + fname + END_TOKEN;
    remote_procedure(line,"create");
}

// Remote procedure call on append
void Shell::append_rpc(string fname, string data) {
    string line = "append " + fname + " " + data + END_TOKEN;  // bugbug add space in 2 places
    remote_procedure(line,"append");
}

// Remote procesure call on cat
void Shell::cat_rpc(string fname) {
    string line= "cat " + fname + END_TOKEN;  
    remote_procedure(line,"cat");
}

// Remote procedure call on head
void Shell::head_rpc(string fname, int n) {
    string line = "head "+ fname + " " + to_string(n)+ END_TOKEN; // bugbug add space
    remote_procedure(line,"head");
}

// Remote procedure call on rm
void Shell::rm_rpc(string fname) {
    string line = "rm " + fname + END_TOKEN;   // bugbug add space
    remote_procedure(line,"rm");
}

// Remote procedure call on stat
void Shell::stat_rpc(string fname) {
    string line= "stat "+ fname + END_TOKEN;  // bugbug add space
    remote_procedure(line,"stat");
}

// Executes the shell until the user quits.
void Shell::run()
{
  // make sure that the file system is mounted
  if (!is_mounted)
 	return; 
  
  // continue until the user quits
  bool user_quit = false;
  while (!user_quit) {

    // print prompt and get command line
    string command_str;
    cout << PROMPT_STRING;
    getline(cin, command_str);

    // execute the command
    user_quit = execute_command(command_str);
  }

  // unmount the file system
  unmountNFS();
}

// Execute a script.
void Shell::run_script(char *file_name)
{
  // make sure that the file system is mounted
  if (!is_mounted)
  	return;
  // open script file
  ifstream infile;
  infile.open(file_name);
  
  if (infile.fail()) 
  {
    cerr << "Could not open script file" << endl;
    return;
  }


  // execute each line in the script
  bool user_quit = false;
  string command_str;
  getline(infile, command_str, '\n');
  
  while (!infile.eof() && !user_quit) 
  {
    cout << PROMPT_STRING << command_str << endl;
    user_quit = execute_command(command_str);
    getline(infile, command_str);
  }

  // clean up
  unmountNFS();
  infile.close();
}

void Shell::shutdown()
{
  close(cs_sock);
  cout << "shutting down..." << endl;
}


// Executes the command. Returns true for quit and false otherwise.
bool Shell::execute_command(string command_str)
{
  // parse the command line
  struct Command command = parse_command(command_str);

  // look for the matching command
  if (command.name == "")
    return false;
  
  else if (command.name == "mkdir")
    mkdir_rpc(command.file_name);
  
  else if (command.name == "cd") 
    cd_rpc(command.file_name);
  
  else if (command.name == "home") 
    home_rpc();
  
  else if (command.name == "rmdir") 
    rmdir_rpc(command.file_name);
  
  else if (command.name == "ls")
    ls_rpc();
  
  else if (command.name == "create") 
    create_rpc(command.file_name);
  
  else if (command.name == "append") 
    append_rpc(command.file_name, command.append_data);
  
  else if (command.name == "cat") 
    cat_rpc(command.file_name);
  
  else if (command.name == "head") 
  {
    errno = 0;
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    
    if (0 == errno)
      head_rpc(command.file_name, n);

    else {
      cerr << "Invalid command line: " << command.append_data;
      cerr << " is not a valid number of bytes" << endl;
      return false;
    }
  }
  
  else if (command.name == "rm") 
    rm_rpc(command.file_name);

  else if (command.name == "stat")
    stat_rpc(command.file_name);
  
  else if (command.name == "quit")
    shutdown();//return true;

  return false;
}

// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
Shell::Command Shell::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct Command empty = {"", "", ""};

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
  if (num_tokens == 0) 
    return empty;
    
  // Check for invalid command lines
  if (command.name == "ls" || command.name == "home" || command.name == "quit")
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


void Shell:: print_response(string command , string response)
{
  stringstream ss(response);
  string item;
  vector<string> halfResponse;

  while(getline(ss,item,'\n'))
    halfResponse.push_back(item);


    if(command == "ls" || command == "head" || command =="stat" || command == "cat")
      for (int ii=3; ii < halfResponse.size(); ii++)  
        cout<<halfResponse[ii]<<endl;

  // print out just the error returned from server
  else
    cout<<halfResponse[0]<<endl;
}

// Read response from the server
void Shell::receive_response(string command)
{
  char line[1024];
  string response;
  int bodyLength  = 0;

  socketHelper.Reset();

  // Server always sends back at least 3 lines
  for (int ii=0; ii < 3; ii++)
  {
    if (socketHelper.ReadLine(line, sizeof(line)) <= 0)
    {
      printf("error reading response from server\n");
      exit(1);
    }
    

    // process the body length (always second line)
    if (ii == 1)
    {
      char *p;
      if ((p = strchr(line, ':')) != NULL) 
        bodyLength = atoi(p+1);
    }
    response += line;
  }

  // now read body
  if (bodyLength)
  {

    socketHelper.ReadLine(line, sizeof(line), bodyLength);
    response += line;
  }

 

  print_response(command, response);
}

void Shell :: remote_procedure(string commandLine, string command)
{
  char sentMessage[2048];
  char received [2048];

  strcpy(sentMessage, commandLine.c_str());

  send(cs_sock,sentMessage, strlen(sentMessage),0);
  receive_response(command);

}
