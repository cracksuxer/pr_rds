//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//                  This class processes an FTP transaction.
// 
//****************************************************************************

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include <unistd.h>

#include "common.h"
#include "ClientConnection.h"


#define COMMAND(cmd) strcmp(command, cmd) == 0

int random(int start, int end) {
  return start+(int)(rand()%(end-start));
}

ClientConnection::ClientConnection(int s, unsigned long ip) {

  int sock = (int)(s);
  char buffer[MAX_BUFF];

  control_socket = s;
  server_address = ip;

  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
    
  if (fd == NULL) {
    std::cout << "Connection closed" << std::endl;
    fclose(fd);
    close(control_socket);
    ok = false;
    return;
  }
    
  ok = true;
  data_socket = -1;
  passive = false;
}

ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
}

int connect_TCP(uint32_t address, uint16_t port) {

  struct sockaddr_in sck_in;

  int sck = socket(AF_INET, SOCK_STREAM, 0);

  if (sck < 0)
    errexit("Could not create specified socket: %s\n", strerror(errno));

  memset(&sck_in, 0, sizeof(sck_in));

  sck_in.sin_family = AF_INET;
  sck_in.sin_addr.s_addr = address;
  sck_in.sin_port = htons(port);

  if (connect(sck, (struct sockaddr*)&sck_in, sizeof(sck_in)) < 0)
    errexit("Could not connet to %s: %s\n", address, strerror(errno));

  return sck;
}

void ClientConnection::WaitForRequests(){

  if (!ok)
    return;
  
  fprintf(fd, "220 Service ready\n");
  printf("\n**************\n* USER: admin *\n* PASS: 1234 *\n**************\n\n");


  while(!parar) {
    fscanf(fd, "%s", command);

    if (COMMAND("USER")) {
      fscanf(fd, "%s", arg);
      printf("USER: %s\n", arg);

      if (!strcmp(arg,"admin"))
        fprintf(fd, "331 User name ok, need password\n");
      else
        fprintf(fd, "332 Need account to make a login\n");
    } 
    
    
    else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      printf("(PASS):%s\n", arg);

      if (!strcmp(arg,"1234"))
        fprintf(fd, "230 User logged in\n");
      else {
        fprintf(fd, "530 Not logged in\n");
        parar = true;
      }
    } 
    
    
    else if (COMMAND("PORT")) {
      passive = false;
    
      unsigned int ip[4];
      unsigned int port[2];

      fscanf(fd, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &port[0], &port[1]);

      uint32_t ip_addr = ip[3] << 24 | ip[2] << 16 | ip[1] << 8 | ip[0];
      uint16_t port_v = port[0] << 8 | port[1];
      
      data_socket = connect_TCP(ip_addr, port_v);

      fprintf(fd, "200 Okey\n");
    } 
    
    // pass
    else if (COMMAND("PASV")) {
      passive = true;

      struct sockaddr_in sck_in, sck_addr;
      socklen_t sa_len = sizeof(sck_addr);
      int sck;

      sck = socket(AF_INET, SOCK_STREAM, 0);

      if (sck < 0)
        errexit("Could not create socket: %s\n", strerror(errno));

      memset(&sck_in, 0, sizeof(sck_in));
      sck_in.sin_family = AF_INET;
      sck_in.sin_addr.s_addr = server_address;
      sck_in.sin_port = random(1024, 65535);

      if (bind(sck, (struct sockaddr*)&sck_in, sizeof(sck_in)) < 0)
        errexit("Could not bind to the specified port: %s\n", strerror(errno));

      if (listen(sck, 5) < 0)
        errexit("Listen error: %s\n", strerror(errno));

      getsockname(sck, (struct sockaddr*)&sck_addr, &sa_len);

      fprintf(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n",
        (unsigned int)(server_address & 0xff),
        (unsigned int)((server_address >> 8) & 0xff),
        (unsigned int)((server_address >> 16) & 0xff),
        (unsigned int)((server_address >> 24) & 0xff),
        (unsigned int)(sck_addr.sin_port & 0xff),
        (unsigned int)(sck_addr.sin_port >> 8));

      data_socket = sck;
    } 
    
    // put
    else if (COMMAND("STOR")) {
      fscanf(fd, "%s", arg);
      printf("STOR: %s\n", arg);
      FILE* file = fopen(arg,"wb");

      if (!file) {
        fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
        close(data_socket);
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);

        struct sockaddr_in sck_addr;
        socklen_t sck_len = sizeof(sck_addr);
        char buffer[MAX_BUFF];
        int recieve_v;

        if (passive)
          data_socket = accept(data_socket,(struct sockaddr *)&sck_addr, &sck_len);

        do {
          recieve_v = recv(data_socket, buffer, MAX_BUFF, 0);
          fwrite(buffer, sizeof(char) , recieve_v, file);
        } while (recieve_v == MAX_BUFF);
    
        fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
        fclose(file);
        close(data_socket);
      }
    } 
    
    // get
    else if (COMMAND("RETR")) {
      fscanf(fd, "%s", arg);
      printf("RETR: %s\n", arg);

      FILE* file = fopen(arg,"rb");

      if (!file) {
        fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
        close(data_socket);
      } else {

        fprintf(fd, "150 File status okay; about to open data connection.\n");

        struct sockaddr_in sck_addr;
        socklen_t sck_len = sizeof(sck_addr);
        char buffer[MAX_BUFF];
        int read_v;

        if (passive)
          data_socket = accept(data_socket,(struct sockaddr *)&sck_addr, &sck_len);

        do {
          read_v = fread(buffer, sizeof(char), MAX_BUFF, file); 
          send(data_socket, buffer, read_v, 0);
        } while (read_v == MAX_BUFF);
                  
        fprintf(fd,"226 Action successful.\n");
        fclose(file);
        close(data_socket);
      }
    } 
    
    // ls
    else if (COMMAND("LIST")) {
      printf("LIST: SHOW\n");
      fprintf(fd, "125 Data connection already open; transfer starting\n");

      struct sockaddr_in sck_addr;
      socklen_t sck_len = sizeof(sck_addr);
      char buffer[MAX_BUFF];
      std::string show;
      std::string ls = "ls -l";

      ls.append(" 2>&1");

      FILE* file = popen(ls.c_str(), "r");

      if (!file) {
        fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
        close(data_socket);
      } else {
        if (passive)
          data_socket = accept(data_socket,(struct sockaddr *)&sck_addr, &sck_len);

        while (!feof(file)) {
          if (fgets(buffer, MAX_BUFF, file) != NULL) 
            show.append(buffer);
        }

        send(data_socket, show.c_str(), show.size(), 0);

        fprintf(fd, "250 Action successful.\n");
        pclose(file);
        close(data_socket);
      }
    } 
    
    
    else if (COMMAND("SYST")) {
      printf("SYST: SHOW\n");
      fprintf(fd, "215 UNIX Type: L8.\n");
    } 
    
    
    else if (COMMAND("TYPE")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "200 OK\n");
    } 
    
    
    else if (COMMAND("QUIT")) {
      fprintf(fd, "221 Service closing control connection. Logged out if appropiate.\n");
      stop();
    } 
    
    // pwd
    else if (COMMAND("PWD")) {
      printf("PWD: SHOW\n");

      char path[MAX_BUFF]; 

      if (getcwd(path, sizeof(path)) != NULL) {
        fprintf(fd, "257 \"%s\" \n", path);
      }
    } 
    
    // cd
    else if (COMMAND("CWD")) {
      fscanf(fd, "%s", arg);
      printf("CWD: %s\n", arg);

      char path[MAX_BUFF];

      if (getcwd(path, sizeof(path)) != NULL) {
        strcat(path,"/");
        strcat(path,arg);

        if (chdir(path) < 0)
          fprintf(fd, "550 Failed to change directory.\n");
        else
          fprintf (fd, "250 Directory successfully changed.\n"); 
      }
    } 
    
    
    else {
      fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }
  
  fclose(fd);
  return;
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}