#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/stat.h>

using namespace std;

void send_error(string err);
bool parse_get_request(string request, string& file_path, string& request_suffix);
bool validate_file(const string& file_path);

void send_error(string err) {
  cout << err << endl;
}

bool parse_get_request(string request, string& file_path, string& request_suffix) {
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

bool validate_file(const string& file_path) {
  // Check to avoid visiting the parent path
  if (file_path.find("../") != string::npos) {
    send_error("bad request - trying to visit parent directory");
    return false;
  }

  // Check permission
  struct stat results;  
  const char *filename_char = file_path.c_str();

  if (stat(filename_char, &results) != 0) {
    send_error("file doesn't exist");
    return false;
  }

  if (!(results.st_mode & S_IROTH)) {
    send_error("file not readable");
  } 
  return true;
}

int main(int argc, char **argv) {


  while (true) {
    string request;
    std::getline(std::cin,request);

    string file_path;
    string request_suffix;

    if (parse_get_request(request, file_path, request_suffix)) {
      if (validate_file(file_path)) {
        // send file.
        cout << "sending file " << file_path << endl;
      } else {
        send_error("GET request failed");
      }
    } else {
      send_error("not a valid GET request");
    }
  }

  return 0;

}
