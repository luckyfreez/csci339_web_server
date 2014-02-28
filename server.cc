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
#include <vector>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <mutex>
#include <sys/syscall.h>   // For support on mac
#include <unistd.h>        // For support on mac


const int TIMEOUT_CONST = 300;

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

  std::cout << "Server initializing..." << std::endl;

  // Create initial socket.
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
  int sock = socket(domain, type, protocol);

  // Allow socket reuse
  int optval = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) exit(1);

  // Bind
  struct sockaddr_in myaddr;
  myaddr.sin_port = htons(port);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) exit(1);
  std::cout << "Binded at socket " << sock << "." << std::endl;

  // Listen
  int backlog = 100;
  if (listen(sock, backlog) < 0) exit(1);
  std::cout << "Started listening" << "." << std::endl;

  // Continually listen to new requests and spawn new threads for each one
  while(true){
    struct sockaddr_in remote;
    unsigned int remotelen = sizeof(remote);
    int temp_sock = accept(sock, (struct sockaddr*)&remote, &remotelen);
    pthread_t thread;
    change_count(1);
    int iret = pthread_create( &thread, NULL, manage_conn, (void*) &temp_sock);
  }
  return 0;
}

void change_count(int delta) {
  count_lock.lock();
  thread_count += delta;
  count_lock.unlock();
}


void *manage_conn(void *ptr) {
  int fd;
  int sock = *((int *) ptr);
  char buf[4096];
  char peek_buf;  // used in the recv to check if a socket is still open
  std::string http_type;

  std::cout << "[Socket " << sock << "] Opened for new connection." << std::endl;
  std::cout << "Number of open connection: " << thread_count << std::endl;

  do {
    // set timeout
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_CONST / thread_count;
    timeout.tv_usec = 0;

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
      send_message(sock, "setsockopt failed\n");
      close(sock);
    }

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
      send_message(sock, "setsockopt failed\n");
      close(sock);
    }

    std::string request = "";
    while (!(request.length() >= 2 && is_char_return(request.at(request.length() - 2)) ) &&
           recv(sock, &peek_buf, 1, MSG_PEEK) >= 0) {
      recv(sock, buf, sizeof(buf), 0);
      std::string tmp(buf);
      tmp = tmp.substr(0, tmp.length() - 1);  // Get rid of the nl (ascii = 10) in the end of each line
      request = request + tmp;
      memset(buf, 0, 4096);  // Empty the buffer
    }
    if (request.length() >= 2 && is_char_return(request.at(request.length() - 2)))
      std::cout << "[Socket " << sock << "] Incoming request - " << request << std::endl;
      parse_request(sock, http_type, request);

  } while (http_type == "HTTP/1.1" && (recv(sock, &peek_buf, 1, MSG_PEEK)) >= 0);

  close(sock);
  change_count(-1);
}

void parse_request(int sock, std::string& http_type, const std::string& request) {
  std::string full_path;
  http_type = "HTTP/1.0";    // Default to 1.0 (for any type that is not HTTP/1.1)

  // Check the initial request line if it is of the form GET [directory] /HTTP{1.0, 1.1}
  std::vector<std::string> tokens = check_initial_request(sock, request);
  if (tokens.size() >= 3 && tokens.at(0) == "GET" && (tokens.at(2) == "HTTP/1.0" || tokens.at(2) == "HTTP/1.1")) {
    if (tokens.at(1) == "/") tokens.at(1) = "/index.html";
    if (tokens.at(2) == "HTTP/1.1") http_type = "HTTP/1.1";
    full_path = document_root + tokens.at(1);

    // Now check for permissions. validate_file method will print out 403 and 404 messages if needed
    if (validate_file(sock, http_type, full_path)) {
      send_file(sock, http_type, full_path);
    }
  } else {
    send_message(sock, http_type + " 400: Bad Request");
    close(sock);
  }
}

std::vector<std::string> check_initial_request(int sock, const std::string& request) {
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
  std::cout << "[Socket " << sock << "] Message - " << message << std::endl;
}

void send_file(int sock, const std::string& http_type, const std::string& file_path) {
  // send the file back
  char buf[1000];
  int nread;
  int fd = open(file_path.c_str(), O_RDONLY);
  if (fd == -1) {
    send_message(sock, http_type + " 500: Internal Server Error");
    close(sock);
    return;
  }

  // Seems to be okay. Print OK status, followed by the date.
  send_message(sock, http_type + " 200 OK");
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
    send_message(sock, "Content-Type: fun stuff");
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
  send_message(sock, "Server: Daniel's and Lucky's Server");

  send_message(sock, "");
  // Now print the HTML
  while ((nread = read(fd, buf, sizeof(buf))) > 0) write(sock, buf, nread);
  close (fd);
  std::cout << "[Socket " << sock << "] " << "Successfully sent file " << file_path << std::endl;
}

bool is_char_return(char c) {
  return (c == '\n' || c == '\r');
}

bool validate_file(int sock, const std::string& http_type, const std::string& file_path) {

  // Check to avoid visiting the parent path
  if (file_path.find("../") != std::string::npos) {
    send_message(sock, http_type + " 403: Forbidden");
    close(sock);
    return false;
  }

  // Check if file exists
  struct stat results;
  const char *filename_char = file_path.c_str();
  if (stat(filename_char, &results) != 0) {
    send_message(sock, http_type + " 404: Not Found");
    close(sock);
    return false;
  }
  // Check permission
  if (!(results.st_mode & S_IROTH)) {
    send_message(sock, http_type + " 403: Forbidden");
    close(sock);
    return false;
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
  return "Date: " + date_local + " (UTC time: " + convert_hour.str() + ":" +
         convert_min.str() + ":" + convert_sec.str() + ")";
}
