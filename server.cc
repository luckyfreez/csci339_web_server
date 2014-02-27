/*
CSCI 339: Distributed Systems, Homework 1
(c) February 2014 by Daniel Seita and Lucky Zhang

Documentation for our methods is in the server.h file. For additional details, see our writeup.
*/

#include "server.h"
#include <stdio.h>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <mutex>


// TODO: Convert char arrays into C++ strings?

const int TIMEOUT_CONST = 10;

std::string document_root;             // File path to prepend to local path
int thread_count;                      // Global count of open threads
std::mutex count_lock;                 // Lock for the global count of open threads


int main(int argc, char **argv) {

  if (argc != 5 || (strcmp(argv[1],"-document_root") != 0 || strcmp(argv[3],"-port") != 0)) {
    std::cout << "Usage: ./server -document_root [file_path] -port [port_num]" << std::endl;
    return -1;
  }
  document_root = argv[2];
  int port = atoi(argv[4]);

  thread_count = 0;

  // Create initial socket.
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
  int sock = socket(domain, type, protocol);
  // std::cout << "sock = " << sock << std::endl;

  // Allow socket reuse
  int optval = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) exit(1);

  // Bind
  struct sockaddr_in myaddr;
  myaddr.sin_port = htons(port);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) exit(1);
  // std::cout << "bind succeeded" << std::endl;

  // Listen
  int backlog = 100;
  if(listen(sock, backlog) < 0) exit(1);
  // std::cout << "listen succeeded" << std::endl;

  // Continually listen to new requests and spawn new threads for each one
  while(true){
    // std::cout << "here at start of loop where we make threads" << std::endl;
    std::cout << "There are " << thread_count << " connecetions open right now" << std::endl;
    struct sockaddr_in remote;
    unsigned int remotelen = sizeof(remote);
    int temp_sock = accept(sock, (struct sockaddr*)&remote, &remotelen);
    // std::cout << "accept a connection (before creating the thread), temp_sock = " << temp_sock << std::endl;
    pthread_t thread;
    int iret = pthread_create( &thread, NULL, manage_conn, (void*) &temp_sock);
    change_count(1);
  }
  return 0;
}

void change_count(int delta) {
  count_lock.lock();
  thread_count += delta;
  count_lock.unlock();
}


// TODO: detect two carriage returns to do this (?)
void *manage_conn(void *ptr) {
  int value = 1;
  int fd, nread, nrecv;
  int sock = *((int *) ptr);
  // std::cout << "sock is " << sock << std::endl;
  char buf[4096];
  std::string http_type;

  std::cout << "Right before receiving message from the buffer" << std::endl;
  //  TODO while loop (if HTTP/1.1, check time out)

  char data;


  do {
    // set timeout
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_CONST / thread_count;
    timeout.tv_usec = 0;

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
          sizeof(timeout)) < 0)
      send_message(sock, "setsockopt failed\n");

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
          sizeof(timeout)) < 0)
      send_message(sock, "setsockopt failed\n");

  // std::string old_buffer_string;
  // std::string new_buffer_string = "";
  // do {
    // old_buffer_string = new_buffer_string;
    // std::cout << "old string: " << old_buffer_string << std::endl;
    if ((nrecv = recv(sock, buf, sizeof(buf), 0)) >= 0) {// Returns number of bytes read in the buffer
    // new_buffer_string = std::string(buf);
    // std::cout << "new string " << new_buffer_string << std::endl;
  // } while (new_buffer_string != old_buffer_string.substr(2));
  // so if the new string is equal to the old string (TAKING AWAY the two leading chars from old string) then it works
  // std::cout << "End of buffer." << std::endl;
      parse_request(sock, http_type, buf);
    }

    std::cout << "Connection still alive  = " << recv(sock, &data, 1, MSG_PEEK) << std::endl;

  } while (http_type == "HTTP/1.1" && (recv(sock, &data, 1, MSG_PEEK)) >= 0);
  close(sock);
  change_count(-1);
}

void parse_request(int sock, std::string& http_type, char* buf) {
  std::string request(buf);
  std::string full_path;
  http_type = "HTTP/1.0";    // Default to 1.0
  std::string request_suffix;

  // Check the initial request line if it is of the form GET [directory] /HTTP{1.0, 1.1}
  std::vector<std::string> tokens = check_initial_request(sock, request);
  if (tokens.size() >= 3 && tokens.at(0) == "GET" && (tokens.at(2) == "HTTP/1.0" || tokens.at(2) == "HTTP/1.1")) {
    if (tokens.at(2) == "HTTP/1.1") {
      http_type = "HTTP/1.1";
    }
    full_path = document_root + tokens.at(1);
    request_suffix = tokens.at(2);

    // Now check for permissions. validate_file method will print out 403 and 404 messages if needed
    if (validate_file(sock, http_type, full_path)) {
      send_file(sock, http_type, full_path);
    }
  } else {
    send_message(sock, "HTTP/1.0 400 Bad Request");
  }
}


std::vector<std::string> check_initial_request(int sock, std::string request) {
  std::stringstream ss(request);
  std::string buf;
  std::vector<std::string> tokens; // Elements here are strings from request, split by whitespaces
  while (ss >> buf) {
    tokens.push_back(buf);
  }
  return tokens;
}


void send_message(int sock, const std::string& message) {
  std::string message_newline = message + "\n"; // Used to get newlines to work
  const char *message_char = message_newline.c_str();
  write(sock, message_char, message_newline.size());
}


void send_file(int sock, const std::string& http_type, const std::string& file_path) {
  // send the file back
  // std::cout << "sending file " << file_path << std::endl;
  char buf[1000];
  int nread;
  int fd = open(file_path.c_str(), O_RDONLY);
  if (fd == -1) {
    send_message(sock, "Error: problem with opening the file"); // TODO change this?
  }

  // Seems to be okay. Print OK status, followed by the date.
  send_message(sock, "\n" + http_type + " 200 OK");
  send_message(sock, current_date_time());

  // Content-Types to support: html, txt, jpg, and gif. Seems like types text/html go together, though.
  // Take file path, split based on periods. then go to the LAST one, which gives us file extension.
  std::vector<std::string> file_path_split = split(file_path, '.');
  std::string file_extension = file_path_split.at(file_path_split.size() - 1);
  if (file_extension == "html" || file_extension == "text") {
    send_message(sock, "Content-Type: text/html");
  } else if (file_extension == "jpg") {
    send_message(sock, "Content-Type: jpg");
  } else if (file_extension == "gif") {
    send_message(sock, "Content-Type: gif");
  } else {
    send_message(sock, "Content-Type: unknown"); // TODO: want to change this?
  }

  // Content-Length
  FILE *p_file = fopen(file_path.c_str(), "rb");
  fseek(p_file, 0, SEEK_END);
  int size = ftell(p_file);
  fclose(p_file);
  std::stringstream sstm;
  sstm << "Content-Length: " << size;
  std::string output = sstm.str();
  send_message(sock, output);

  // Some optional stuff
  send_message(sock, "Server: Daniel's Server");

  // Now print the HTML
  send_message(sock, "");
  while ((nread = read(fd, buf, sizeof(buf))) > 0)
    write(sock, buf, nread);
  close (fd);
}


bool validate_file(int sock, const std::string& http_type, const std::string& file_path) {

  // Check to avoid visiting the parent path
  if (file_path.find("../") != std::string::npos) {
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


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::string current_date_time() {
  std::string return_date = "Date: ";
  time_t now = time(0);
  char* dt = ctime(&now);
  tm *gmtm = gmtime(&now);
  std::string date_local = std::string(dt);
  date_local = date_local.erase(date_local.find_last_not_of(" \n\r\t") + 1);

  // Now put the UTC time (for completeness/clarity)
  int hour = gmtm->tm_hour;
  int min = gmtm->tm_min;
  int sec = gmtm->tm_sec;
  std::ostringstream convert_hour;
  std::ostringstream convert_min;
  std::ostringstream convert_sec;
  convert_hour << hour;
  convert_min << min;
  convert_sec << sec;
  return "Date: " + date_local + " (UTC time: " + convert_hour.str() + ":" + convert_min.str() + ":" + convert_sec.str() + ")";
}
