#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "registro.h"

#define DIRECCION_IP_SERVER "127.0.0.1"
#define DIRECCION_IP_PROPIA "127.0.0.1"

int main(int argc, char* argv[]){
    //DECLARACIÓN DE VARIABLES Y PUNTEROS
    int fdsocket, clienteAbierto=1, cantMensajes;
    char* msg = NULL; //DARLE UN TAMAÑO COHERENTE PARA EL ENVÍO DE MENSAJES EN EL ARCHIVO REGISTRO.H
    struct sockaddr_in server_info;
    size_t tamMsg=TAM_MSG;

    msg = malloc(tamMsg * sizeof(char)); //
    //MANEJO DE SEÑALES PARA SUPRIMIR EL CTRL+C
    signal(SIGINT,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);

    //INICIALIZACIÓN DE SOCKET
    fdsocket = socket(AF_INET,SOCK_STREAM,0);

    //VERIFICACIÓN DE INICIALIZACIÓN
    if(fdsocket==-1){
      printf("Hubo un error al abrir el socket\n");
      free(msg);
      return -1;
    }
    printf("[Cliente] -> Socket iniciado correctamente\n");

    //INICIALIZAR INFORMACIÓN DEL SERVIDOR
    server_info.sin_family=AF_INET;
    server_info.sin_port=htons(50001);
    server_info.sin_addr.s_addr=inet_addr(DIRECCION_IP_SERVER);


    //CONEXIÓN DEL SOCKET CLIENTE CON EL SERVIDOR
    if(connect(fdsocket,(struct sockaddr *) &server_info,sizeof(struct sockaddr_in))==0){
      printf("[Cliente] -> El cliente esta listo\n");

      //SI EL SERVIDOR NO ACEPTA MÁS CONEXIONES EL LADO DEL CLIENTE, EL MISMO TERMINA LA CONEXIÓN Y CIERRA, EL MENSAJE ES EL ESTADO DEL SERVIDOR
      if( !( (int)recv(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0) ) ){
        puts("No se pudo establecer conexión con el servidor, intentelo más tarde");
        free(msg);
        close(fdsocket);
        return -1;
      }

      //IMPRIME EL MENSAJE DE ESTADO DEL SERVIDOR
      printf("%s\n",msg);

      //RECIBE UN MENSAJE DEL ESTADO DE LA CONEXIÓN, SOCKETS LIBRES U SOCKETS EN LISTA DE ESPERA
      if( (int)recv(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0) < 0){
        puts("No se pudo establecer conexión con el servidor, intentelo más tarde");
        close(fdsocket);
        return -1;
      }

      //SI EL PRIMER CARACTER ES 1 ENTONCES DEBE ESPERAR A QUE SE LIBERE UN SOCKET
      if(msg[0]=='1'){
        if( !( (int)recv(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0) ) ){
          puts("No se pudo establecer conexión con el servidor, intentelo más tarde");
          close(fdsocket);
          return -1;
        }
      }
      //SI EL PRIMER CARACTER ES 2 ENTONCES ESTABLECE LA CONEXIÓN, SI NO, NO FUÉ UN MENSAJE DEL SERVIDOR
      else if(msg[0]!='2'){
        puts("No se pudo establecer conexión con el servidor, intentelo más tarde");
        close(fdsocket);
        return -1;
      }
      puts("[Cliente] -> Conexión establecida con el servidor");

      //BUCLE PRINCIPAL DEL CLIENTE
      while(clienteAbierto){
        //RECIBE CUANTOS MENSAJES SE RECIBIRÁN
        if(recv(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0)==0){
          puts("Se perdió la conexión con el servidor");
          clienteAbierto=0;
        }
        puts(msg);//IMPRIME EL MENSAJE RECIBIDO
        if(clienteAbierto){
          cantMensajes = strtol(msg,NULL,10);//GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A RECIBIR

          //WHILE PARA RECIBIR TODOS LOS MENSAJES PENDIENTES
          while(cantMensajes){
            //ESPERA UN MENSAJE DEL SERVIDOR, SI RECIBE 0 (CIERRE DEL SERVIDOR) ENTONCES TERMINA LA EJECUCIÓN SE PUEDE CAMBIAR POR UNA ESPERA
            //EN BASE A UN TIEMPO Y VOLVER A PEDIR CONEXIÓN
            if(recv(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0)==0){
              puts("Se perdió la conexión con el servidor");
              clienteAbierto=0;
            }
            puts(msg);//IMPRIME EL MENSAJE RECIBIDO
            cantMensajes--;
          }
        }


        if(clienteAbierto){
          //PIDE UN MENSAJE AL CLIENTE PARA SER ENVIADO AL SERVIDOR
          puts("[Cliente] -> Ingrese un mensaje:");
          fflush(stdin);
          if(getline(&msg,&tamMsg,stdin)>0){ //SI NO SE PUDO LEER LA LINEA NO ENVIA EL MENSAJE NI ESPERA UNO NUEVO
            limpiarSalto(msg);//LIMPIA EL SALTO FINAL DEL GETLINE

            //ENVIA EL MENSAJE AL SERVIDOR
            printf("[Cliente] -> Enviando: %s\n",msg);
            if(send(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0) == -1){
              puts("FALLO");
              clienteAbierto=0;
            }
          }
        }

      }
    }
    //FALLO DE CONEXIÓN
    else
      printf("Hubo un error intentando conectar el cliente al servidor\n");

    //LIBERACIÓN DE RECURSOS
    free(msg);
    close(fdsocket);
    return 0;
}
