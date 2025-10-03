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
    printf("[Cliente] -> Socket iniciado correctamente\n");

    server_info.sin_family=AF_INET;
    server_info.sin_port=htons(50001);
    server_info.sin_addr.s_addr=inet_addr(DIRECCION_IP_SERVER);



    if(connect(fdsocket,(struct sockaddr *) &server_info,sizeof(struct sockaddr_in))==0){
      printf("[Cliente] -> El cliente esta listo\n");
      while(1){
        if(recv(fdsocket,(void*)&msg,sizeof(msg),0)==0){
          puts("No se pudo establecer conexión con el servidor, intentelo más tarde");
          break;
        }
        printf("[Cliente] -> Recibido: %s\n",msg);


        printf("[Cliente] -> Ingrese un mensaje:\n");

        fflush(stdin);
        scanf("%s", msg);

        printf("[Cliente] -> Enviando: %s\n",msg);
        if(send(fdsocket, (void*) &msg, sizeof(msg),0) == -1)
          puts("FALLO");

      }
    }
    else
        printf("Hubo un error intentando conectar el cliente al servidor\n");

    close(fdsocket);
    return 0;
}
