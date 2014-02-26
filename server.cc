#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>

using namespace std;

string file_path;
void *manage_conn(void *ptr); 
void parse_request(int sock, char *buf);
void send_error(int sock, const string& err);
void send_file(int sock, const string& file_path);
bool parse_get_request(int sock, string request, string& file_path, string& request_suffix);
bool validate_file(int sock, const string& file_path);

int main(int argc, char **argv) {

  // For dealing with command line arguments
  if (argc != 5 || (strcmp(argv[1],"-document_root") != 0 || strcmp(argv[3],"-port") != 0)) {
    cout << "Usage: ./server -document_root [file_path] -port [port_num]" << endl;
    return -1;
  }
  file_path = argv[2];
  int port = atoi(argv[4]);

  // Vector of threads
  vector<pthread_t> threads;

  // Create initial socket.
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
  int sock = socket(domain, type, protocol);
  cout << "sock = " << sock << endl;

  // Now allow socket reuse
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
    cout << "accept a connection (before creating the thread), temp_sock = " << temp_sock << endl;
    pthread_t thread;
    int iret = pthread_create( &thread, NULL, manage_conn, (void*) &temp_sock);
    threads.push_back(thread); 
    pthread_join(thread, NULL);
  }
  return 0;
}


void *manage_conn(void *ptr) { 
  int value = 1;
  int fd, nread, nrecv;
  int temp_sock = *((int *) ptr);
  cout << "temp_sock is " << temp_sock << endl;
  char buf[1024];  //C has no “String” class
 
  while (true) {
    nrecv = recv(temp_sock, buf, sizeof(buf), 0);

    parse_request(temp_sock, buf);

  }
}

void parse_request(int sock, char* buf) {
  string request(buf);

  string file_path;
  string request_suffix;

  if (parse_get_request(sock, request, file_path, request_suffix)) {
    if (validate_file(sock, file_path)) {
      // send file.
      send_file(sock, file_path);
    } else {
      send_error(sock, "GET request failed");
    }
  } else {
    send_error(sock, "not a valid GET request");
  }
}

void send_error(int sock, const string& err) {
  const char *err_char = err.c_str();
  write(sock, err_char, err.size());
  write(sock, "\n", err.size());
}

void send_file(int sock, const string& file_path) {
  // send the file back
  cout << "sending file " << file_path << endl;
  char buf[1000];
  int nread;
  int fd = open(file_path.c_str(), O_RDONLY);
  if (fd == -1) {
    send_error(sock, "error opening the file");
  }
  while ((nread = read(fd, buf, sizeof(buf))) > 0) 
    write(sock, buf, nread);
  close (fd);
}

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
    cout << "after taking out \'GET\', request = \'" << request << "\'" << endl;

    // Parse the file name
    if ((space_index = request.find(" ")) > 0 and request.size() >= space_index + 1) {
      file_path = document_root + request.substr(0, space_index);
      request_suffix = request.substr(space_index + 1);
      return true;
    } 
  }
  return false;
}

bool validate_file(int sock, const string& file_path) {
  // Check to avoid visiting the parent path
  if (file_path.find("../") != string::npos) {
    send_error(sock, "bad request - trying to visit parent directory");
    return false;
  }

  // Check permission
  struct stat results;  
  const char *filename_char = file_path.c_str();

  if (stat(filename_char, &results) != 0) {
    send_error(sock, "file doesn't exist");
    return false;
  }

  if (!(results.st_mode & S_IROTH)) {
    send_error(sock, "file not readable");
  } 
  return true;
}

/*
  string document_root = "~/cs339/csci339_web_server";
  string request = "GET /index.html HTTP/1.0";
  string rel_file_path, abs_file_path;
  int space_index;

  if (request.find("GET") == 0 and request.size() >= 4) {
    
    // The request string starts with "GET".
    request = request.substr(4);
    cout << "after taking out \'GET\', request = \'" << request << "\'" << endl;

    // Parse the file name
    if ((space_index = request.find(" ")) > 0 and request.size() >= space_index + 1) {
        rel_file_path = request.substr(0, space_index);
        request = request.substr(space_index + 1);
        abs_file_path = document_root + rel_file_path;
        cout << "after taking out the rel file path, request = \'" << request << "\'" << endl;
        cout << "abs_file_path = \'" << abs_file_path << "\'" << endl;
    }
  }
*/ 
