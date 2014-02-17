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

  // Create a socket.
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
  int sock = socket(domain, type, protocol);

  // Bind 
  struct sockaddr_in myaddr;

  myaddr.sin_port = htons(8101);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if(bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr))<0) exit(1);

  int temp_sock;

  char buf[1024];  //C has no “String” class
  int nrecv;


    // Listen
    int backlog = 100;
    listen(sock, backlog);

    // Accept
    unsigned int temp = sizeof(myaddr);
    temp_sock = accept(sock, (struct sockaddr*)&myaddr, (unsigned int *)temp);

    //while ((nrecv = recv(temp_sock, buf, sizeof(buf), 0)) > 0) {
      ////write(1, buf, nrecv);
    //}

  while (true) {
    while (true) {
      std::cout << "1";
      while ((nrecv = recv(temp_sock, buf, sizeof(buf), 0)) > 0) {
        write(1, buf, nrecv);
      }
    }


    //close (temp_sock);

  }
  
  return 0;

}
