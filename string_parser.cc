#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>

using namespace std;

int main(int argc, char **argv) {
  
  /*
  int fd, nread;
  char buf[1024];  //C has no “String” class
  
  fd = open("a.txt", O_RDONLY);

  if (fd == -1) {
    cout << "error" << endl;
    return -1; //error opening file
  }

  while ((nread = read(fd, buf, sizeof(buf))) > 0) 
    write(1, buf, nread);
  close (fd);
  */

/********************************************************************/ 

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
  
  return 0;

}
