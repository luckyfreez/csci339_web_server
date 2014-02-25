#include <iostream>
#include <cstdlib>
#include <fcntl.h>
#include <vector>
#include <netinet/in.h>

using namespace std;


void *manage_conn(void *ptr); 

int main(int argc, char **argv) {

  // Vector of threads
  vector<pthread_t> threads;

  // Create initial socket.
  int domain = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
  int sock = socket(domain, type, protocol);
  cout << "sock = " << sock << endl;

  // Bind 
  struct sockaddr_in myaddr, remote;
  myaddr.sin_port = htons(8899);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sock, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) exit(1);
  cout << "bind succeeded" << endl;

  // Listen
  int backlog = 100;
  if(listen(sock, backlog) < 0) exit(1);
  cout << "listen succeeded" << endl;

  // Continually listen to new requests and spawn new threads for each one
  while(true){
    unsigned int remotelen = sizeof(remote);
    cout << "here" << endl;
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
      cout << "Here is nrecv: " << nrecv << endl;
      write(temp_sock, buf, nrecv);
    cout << "finish reading one request" << endl;

    // send the index.html back
    fd = open("index.html", O_RDONLY);
    if (fd == -1) {
      cout << "error" << endl;
      //return (void*)&value; //error opening file
    }
    while ((nread = read(fd, buf, sizeof(buf))) > 0) 
      write(temp_sock, buf, nread);
    close (fd);
  }

} 
