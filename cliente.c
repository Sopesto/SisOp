#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DIRECCION_IP_SERVER "127.0.0.1"
#define DIRECCION_IP_PROPIA "127.0.0.1"

int main(int argc, char* argv[]){
    int fdsocket;
    char msg[101];
    struct sockaddr_in server_info;

    fdsocket = socket(AF_INET,SOCK_STREAM,0);

    if(fdsocket==-1){
      printf("Hubo un error al abrir el socket\n");
      return 1;
    }
    printf("[Cliente] -> socket iniciado correctamente\n");

    server_info.sin_family=AF_INET;
    server_info.sin_port=htons(50001);
    server_info.sin_addr.s_addr=inet_addr(DIRECCION_IP_SERVER);

    if(connect(fdsocket,(struct sockaddr *) &server_info,sizeof(struct sockaddr_in))==0){
        printf("[Cliente] -> El cliente esta conectado\n");
        printf("[Cliente] -> Ingrese un mensaje (hola para salir):\n");

        do{
          fflush(stdin);
          scanf("%s", msg);
          fflush(stdin);
          printf("[Cliente] -> Enviando %s\n",msg);
          write(fdsocket,(void*)&msg,sizeof(msg));
          read(fdsocket,(void*)&msg,sizeof(msg));
          printf("[Cliente] -> Recibido %s\n",msg);
        }while(msg[0]!='0');

    }
    else
      printf("Hubo un error intentando conectar el cliente al servidor\n");



    close(fdsocket);
    return 0;
}
