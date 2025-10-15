#include "registro.h"

int esDireccionValida(char*);

int main(int argc, char* argv[]){
    //DECLARACIÓN DE VARIABLES Y PUNTEROS
    int fdsocket, clienteAbierto=1, cantMensajes, puerto, i;
    char* msg = NULL; //DARLE UN TAMAÑO COHERENTE PARA EL ENVÍO DE MENSAJES EN EL ARCHIVO REGISTRO.H
    char* ip = NULL;
    struct sockaddr_in server_info;
    size_t tamMsg=TAM_MSG;

    //EXTRAER PARÁMETROS
    if(argc<3){
    puts("La cantidad de argumentos es insuficiente.");
    puts("Argumentos: 1-IP del servidor 2-Puerto del servidor.");
    return ERR_ARG;
    }
    if(argc>3){
      puts("La cantidad de argumentos es mayor a la requerida.");
      puts("Argumentos: 1-IP del servidor 2-Puerto del servidor.");
      return ERR_ARG;
    }

    for(i=0;(argv[2])[i] != '\0';i++){
      if(isalpha((argv[2])[i])){
        puts("Los argumentos son del tipo incorrecto.");
        puts("Argumentos: 1-IP del servidor 2-Puerto del servidor.");
        return ERR_ARG;
      }
    }

    //CONVERSIÓN DE TEXTO A NÚMERO DE LOS PARÁMETROS
    puerto = strtol(argv[2],NULL,10);
    ip = argv[1];

    //COMPROBACIÓN DE RANGO
    if(puerto<0 || puerto > 65535){
      puts("El puerto debe ser un número en el rango entre 0 y 65535.");
      puts("Argumentos: 1-IP del servidor 2-Puerto del servidor.");
      return ERR_ARG;
    }

    //COMPROBACIÓN DE IP
    if(!esDireccionValida(ip)){
      puts("La IP debe se válida");
      puts("Argumentos: 1-IP del servidor 2-Puerto del servidor.");
      return ERR_ARG;
    }


    //INICIALIZAR INFORMACIÓN DEL SERVIDOR
    server_info.sin_family=AF_INET;
    server_info.sin_port=htons(puerto);
    server_info.sin_addr.s_addr=inet_addr(ip);

    //INICIALIZACIÓN DE SOCKET
    fdsocket = socket(AF_INET,SOCK_STREAM,0);

    msg = malloc(tamMsg * sizeof(char)); //

    //MANEJO DE SEÑALES PARA SUPRIMIR EL CTRL+C
    signal(SIGINT,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);

    //VERIFICACIÓN DE INICIALIZACIÓN
    if(fdsocket==-1){
      printf("Hubo un error al abrir el socket\n");
      close(fdsocket);
      free(msg);
      return -1;
    }
    printf("[Cliente] -> Socket iniciado correctamente\n");

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

        if(clienteAbierto){
          cantMensajes = strtol(msg,NULL,10);//GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A RECIBIR
          if(cantMensajes==0)
            clienteAbierto=0;
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

            if((int)tamMsg>TAM_MSG){
              *(msg+TAM_MSG-1) = '\0';
              msg = realloc(msg, TAM_MSG * sizeof(char));
              tamMsg = TAM_MSG;
            }

            //ENVIA EL MENSAJE AL SERVIDOR
            printf("[Cliente] -> Enviando: %s\n",msg);
            if(send(fdsocket,(void*)msg,sizeof(char) * TAM_MSG,0) == -1){
              puts("FALLO");
              clienteAbierto=0;
            }
            usleep(100);
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

int esDireccionValida(char* ip){
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip, &(sa.sin_addr));
    return result > 0;
}
