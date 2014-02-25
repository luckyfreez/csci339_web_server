#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <stdlib.h>

void *print_message_function( void *ptr );

main()
{
  pthread_t thread1, thread2;
  int iret1, iret2;

  const char *message1 = "Thread 1";
  const char *message2 = "Thread 2";

  
  iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
  iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) message2);

  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

}

void *print_message_function( void *ptr )
{
  while (true) {
    std::cout << (char *)ptr << std::endl;
  }
}
