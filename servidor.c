#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include "registro.h"
//#include "palabra.h"

#define DIRECCION_IP_SERVER "127.0.0.1"
#define DIRECCION_IP_PROPIA "127.0.0.1"

int main(int argc, char* argv[]){
    int fdsocket, fdcliente, valor=1;
    char msg[101];
    struct sockaddr_in socket_info;

    fdsocket = socket(AF_INET,SOCK_STREAM,0);

    if(fdsocket==-1){
      printf("Hubo un error al abrir el socket\n");
      return 1;
    }
    printf("[Server] -> socket iniciado correctamente\n");

    socket_info.sin_family=AF_INET;
    socket_info.sin_port=htons(50001);
    socket_info.sin_addr.s_addr=inet_addr(DIRECCION_IP_PROPIA);

    if(bind(fdsocket,(struct sockaddr*) &socket_info,sizeof(struct sockaddr_in))==-1){
      printf("Hubo un error al realizar el bind\n");
      return 2;
    }
    printf("[Server] -> bind realizado correctamente\n");

    setsockopt(fdsocket, SOL_SOCKET, SO_REUSEADDR,(const void*) &valor,sizeof(int));

    if(listen(fdsocket,1)!=0){
      printf("[Server] -> Ocurrio un error al escuchar por el socket\n");
      return 3;
    }

    printf("[Server] -> A la espera de un nuevo cliente\n");
    fdcliente = accept(fdsocket,NULL,NULL);

    while(read(fdcliente, (void*) &msg, sizeof(msg))>0){
      printf("[Server] -> Mensaje leido %s\n", msg);
      if(msg[0]=='h')
        msg[0]='0';
      printf("[Server] -> Respuesta %s", msg);
      write(fdcliente, (void*) &msg, sizeof(char*));
    }

    close(fdcliente);
    close(fdsocket);
    return 0;
}
