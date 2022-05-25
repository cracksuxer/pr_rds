#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdio>
#include <cstdint>


const int MAX_BUFF = 512;


class ClientConnection{
 public:
  ClientConnection(int s, unsigned long ip);
  ~ClientConnection();

  void WaitForRequests();
  void stop();

 private:
  bool ok;    // This variable is a flag that avois that the
          // server listens if initialization error ocurred

  FILE *fd;   // C file desciptor. We useit to buffer the
      // control connection of the socket and it allows to
      // manage it as a C file using fprintf, fscanf, etc.

  char command[MAX_BUFF];   // Buffer for saving the command
  char arg[MAX_BUFF];       // Buffer for savinf the arguments
  char arg2[MAX_BUFF];      // Second buffer for savinf the arguments

  int data_socket;          // Data socket descriptor
  int control_socket;       // Control socket descriptor
  bool parar;

  unsigned long server_address;
  bool passive;
};

#endif