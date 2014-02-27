/*
CSCI 339: Distributed Systems
Project 1: Web Server
(c) February 2014, by Daniel Seita and Lucky Zhang

For details beyond what the comments can cover, see our writeup.

Functions should be declared in the same order that they appear in server.cc (hopefully)
*/


#ifndef SERVER 
#define SERVER
#include <string>
#include <vector>
using namespace std; // TODO remove! Sorry for including this here ...


/*
For a given connection/thread, we take in the request and parse it.
*/
void *manage_conn(void *ptr);  


/*
  Given a request, we parse check for correctness. If initial request is malformed, return HTTP 400 error. 
  Otherwise, move on to validating the given file path, which calls other methods that return HTTP statuses.
*/
void parse_request(int sock, char *buf);


/*
  Called to check the first line for the client's request, i.e. the GET command. Checks the 1st and 3rd
  args, which should be GET and the HTTP type. We assume the 2nd is file path and stuff after are  headers.
  If it looks okay, we set the correct HTTP protocol to use and don't return error (yet).
*/
vector<string> check_initial_request(int sock, string request);


/*
  Sends message to the client-side, to see the direct results of his/her actions.
  Use this to send HTTP status codes, headers, etc.
*/
void send_message(int sock, const string& message);


/*
  Given a file request from the client, open it and send the HTTP response (if can't open file, error).
  HTTP response includes (at least) the status, date, content-type, content-length, and the actual file.
*/ 
void send_file(int sock, const string& file_path);


/*
  Checks if the file exists, and if it is readable. This can output HTTP 403 and 404 statuses.
*/
bool validate_file(int sock, const string& file_path);


/*
  Helper method for the string splitting function
*/
vector<string> &split(const string &s, char delim, vector<string> &elems);


/*
  String splitting based on a given delimeter (typically periods or whitespaces)
*/
vector<string> split(const string &s, char delim);


/*
  Current date/time. Outputs local time/date, but afterwards puts the UTC time for completeness.
  Format: [Dayofweek] [Month] [Day] [Hours:Mins:Secs] [Year] ([UTC Time])
*/
string current_date_time();

#endif
