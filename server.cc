/*
CSCI 339: Distributed Systems, Homework 1
(c) February 2014 by Daniel Seita and Lucky Zhang
For details, see our writeup.
*/

// TODO: Convert char arrays into C++ strings?


#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>

// Used for string parsing
#include <sstream>
#include <algorithm>
#include <iterator>


using namespace std;

// TODO: Documentation, and get a header file set up, so we don't have to put all this up here.

string document_root;
string http_type = "HTTP/1.0"; // Default to 1.0
void *manage_conn(void *ptr); 
void parse_request(int sock, char *buf);
void send_message(int sock, const string& message);
void send_file(int sock, const string& file_path);
bool parse_get_request(int sock, string request, string& file_path, string& request_suffix);
bool validate_file(int sock, const string& file_path);


int main(int argc, char **argv) {

  if (argc != 5 || (strcmp(argv[1],"-document_root") != 0 || strcmp(argv[3],"-port") != 0)) {
    cout << "Usage: ./server -document_root [file_path] -port [port_num]" << endl;
    return -1;
  }
  document_root = argv[2];
  int port = atoi(argv[4]);

  // Vector of threads
  vector<pthread_t> threads;

  // Create initial socket.
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
  int sock = socket(domain, type, protocol);
  // cout << "sock = " << sock << endl;

  // Allow socket reuse
  int optval = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) exit(1);

  // Bind 
  struct sockaddr_in myaddr, remote;
  myaddr.sin_port = htons(port);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) exit(1);
  // cout << "bind succeeded" << endl;

  // Listen
  int backlog = 100;
  if(listen(sock, backlog) < 0) exit(1);
  // cout << "listen succeeded" << endl;

  // Continually listen to new requests and spawn new threads for each one
  while(true){
    unsigned int remotelen = sizeof(remote);
    // cout << "here" << endl;
    int temp_sock = accept(sock, (struct sockaddr*)&remote, &remotelen);
    // cout << "accept a connection (before creating the thread), temp_sock = " << temp_sock << endl;
    pthread_t thread;
    int iret = pthread_create( &thread, NULL, manage_conn, (void*) &temp_sock);
    threads.push_back(thread); 
    pthread_join(thread, NULL);
  }
  return 0;
}


// TODO: detect two carriage returns to do this
void *manage_conn(void *ptr) { 
  int value = 1;
  int fd, nread, nrecv;
  int temp_sock = *((int *) ptr);
  // cout << "temp_sock is " << temp_sock << endl;
  char buf[4096];
  while (true) {
    nrecv = recv(temp_sock, buf, sizeof(buf), 0); // Returns number of bytes read in the buffer
    parse_request(temp_sock, buf);
  }
}


// Assumes we are using GET only. Checks the first and third arguments.
// If it looks okay, we set the correct HTTP protocol to use. Splits string based on whitespaces
vector<string> check_initial_request(int sock, string request) {
  stringstream ss(request);
  string buf;
  vector<string> tokens;
  while (ss >> buf) {
    tokens.push_back(buf);
  }
  return tokens;
}


// Given a request by the client, parse it and check for correctness, etc.
// Sends back status codes, supporting 200, 400, 403, and 404 status codes
// Headers: support Content-Type, Content-Length, and Date headers
// File types: HTML, TXT, JPG, and GIF files (not for this method)
void parse_request(int sock, char* buf) {
  string request(buf);
  string full_path;
  string request_suffix;

  // Check the initial request line if it is of the form GET [directory] /HTTP{1.0, 1.1}
  vector<string> tokens = check_initial_request(sock, request);

  if (tokens.size() >= 3 && tokens.at(0) == "GET" && (tokens.at(2) == "HTTP/1.0" || tokens.at(2) == "HTTP/1.1")) {
    if (tokens.at(2) == "HTTP/1.1") {
      http_type = "HTTP/1.1";
    }
    full_path = document_root + tokens.at(1);
    request_suffix = tokens.at(2);
 
    // Now check for permissions. validate_file method will print out 403 and 404 messages if needed
    if (validate_file(sock, full_path)) { // TODO
      send_message(sock, http_type + " 200 OK");
      send_file(sock, full_path);
    } 
  } else {
    send_message(sock, "HTTP/1.0 400 Bad Request"); // TODO: modify for type 1.0 1.1?
  }

  // After request, default to original values

  // Sources we read suggest closing after each client request. (TODO: Actually, this gives me some problems...)
  // close(sock);
}


// Sends message to the client-side, to see the direct results of his/her actions
void send_message(int sock, const string& message) {
  string message_newline = message + "\n";
  const char *message_char = message_newline.c_str();
  write(sock, message_char, message_newline.size());
  // write(sock, "\n", message.size());
}


void send_file(int sock, const string& file_path) {
  // send the file back
  // cout << "sending file " << file_path << endl;
  char buf[1000];
  int nread;
  int fd = open(file_path.c_str(), O_RDONLY);
  if (fd == -1) {
    send_message(sock, "Error: problem with opening the file");
  }
  while ((nread = read(fd, buf, sizeof(buf))) > 0) 
    write(sock, buf, nread);
  close (fd);
}



// Now check if it exists and, furthermore, if it is readable
bool validate_file(int sock, const string& file_path) {

  // Check to avoid visiting the parent path
  if (file_path.find("../") != string::npos) {
    send_message(sock, "Error: bad request -- trying to visit parent directory"); // TODO: Make this HTTP request?
    return false;
  }

  // Check permission
  struct stat results;  
  const char *filename_char = file_path.c_str();

  if (stat(filename_char, &results) != 0) {
    send_message(sock, http_type + " 404: Not Found");
    return false;
  }
  if (!(results.st_mode & S_IROTH)) {
    send_message(sock, http_type + " 403: Forbidden");
  } 
  return true;
}


/*
// Checks if the GET request is well formed
bool parse_get_request(int sock, string request, string& file_path, string& request_suffix) {
  //string document_root = "~/cs339/csci339_web_server";
  //string document_root = "/home/cs-faculty/fern";
  string document_root = ".";
  string rel_file_path, abs_file_path;
  int space_index;
  file_path = "joi";

  if (request.find("GET") == 0 and request.size() >= 4) {
    // The request string starts with "GET".
    request = request.substr(4);
    cout << "after taking out \'GET\', request = " << request << endl;

    // Parse the file name, and modify protocol to be 1.1 if necessary
    if ((space_index = request.find(" ")) > 0 and request.size() >= space_index + 1) {
      file_path = document_root + request.substr(0, space_index);
      request_suffix = request.substr(space_index + 1);
      return true;
    } 
  }
  return false;
}
*/
