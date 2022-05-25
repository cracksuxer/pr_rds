//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//                          Main class of the FTP server
// 
//****************************************************************************

#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>

#include <pthread.h>

#include <list>

#include "common.h"
#include "FTPServer.h"
#include "ClientConnection.h"



int define_socket_TCP(int port){

    struct sockaddr_in sin;
    int sck = socket(AF_INET, SOCK_STREAM, 0);

    if (sck < 0)
        errexit("Could not create socket: %s\n", strerror(errno));

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(sck, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        errexit("Could not make bind to specified port: %s\n", strerror(errno));

    if (listen(sck,5) < 0)
        errexit("Listen error: %s\n", strerror(errno));
  
    return sck;
}

// This function is executed when the thread is executed.
void* run_client_connection(void *c){
  ClientConnection *connection = (ClientConnection *)c;
  connection->WaitForRequests();

  return NULL;
}

FTPServer::FTPServer(int port){
  this->port = port;
}

// Parada del servidor.
void FTPServer::stop(){
  close(msock);
  shutdown(msock, SHUT_RDWR);
}

// Starting of the server
void FTPServer::run(){

  struct sockaddr_in fsin;
  int ssock;
  socklen_t alen = sizeof(fsin);

  msock = define_socket_TCP(port);
  
  while (1){
    pthread_t thread;
    ssock = accept(msock, (struct sockaddr *)&fsin, &alen);

    if(ssock < 0)
      errexit("Fallo en el accept: %s\n", strerror(errno));

    ClientConnection *connection = new ClientConnection(ssock,fsin.sin_addr.s_addr);

    // Here a thread is created in order to process multiple
    // requests simultaneously
    pthread_create(&thread, NULL, run_client_connection, (void*)connection);
  }
}

