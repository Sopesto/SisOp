#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "registro.h"

#define DIRECCION_IP_SERVER "127.0.0.1"
#define DIRECCION_IP_PROPIA "127.0.0.1"

//VARIABLES GLOBALES Y SEMÁFOROS
//archivo_en_uso se usa para avisar al cliente que el archivo está en uso sin bloquear al usuario
int   archivo_en_uso = 0, socketsLibres, sockEspera=0, estadoServidor=1;
int*  socketsClientes, *socketsEspera;
pthread_mutex_t     semSocketLibre = PTHREAD_MUTEX_INITIALIZER, semArchUso = PTHREAD_MUTEX_INITIALIZER, semCreaT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t*    estadosHilos;

void* hilo_socket(void*);
int   buscarSocketLibre(int*,int);

int main(int argc, char* argv[]){
    //CREACIÓN DE VARIABLES
    int                 fdsocket, auxAccept, valor=1, cantHilos=3, cantEspera=2, i;
    struct sockaddr_in  socket_info;
    pthread_t*          hilos;
    //FILE*               archivoRegistros;
    char                msg_esp[]="Servidor lleno, espera en cola";

    //MANEJO DE SEÑALES
    struct sigaction act; //PARA EL MANEJO DE SEÑALES
    act.sa_handler=manejadorInterrupciones;
    sigaction(SIGINT,&act,NULL);

    //EXTRAER PARÁMETROS
    if(verifParams(argv,argc,3,isalpha,"Argumentos: 1-Cantidad de clientes simultáneos 2-Cantidad de clientes en espera.") == ERR_ARG)
      return ERR_ARG;

    //CONVERSIÓN DE TEXTO A NÚMERO DE LOS PARÁMETROS
    cantHilos   = strtol(argv[1],NULL,10);
    cantEspera  = strtol(argv[2],NULL,10);

    //COMPROBACIÓN DE RANGO
    if(cantHilos<=0 || cantEspera<=0){
      puts("Los argumentos deben ser números positivos mayores a 0.");
      puts("Argumentos: 1-Cantidad de clientes simultáneos 2-Cantidad de clientes en espera.");
      return ERR_ARG;
    }

    /*
    //ABRIR ARCHIVO
    if(abrir_archivo(&archivoRegistros,"","")==-1){
      puts("Error al abrir el archivo de registros");
      return ERR_ARCH;
    }
    */

    //ABRIR SOCKET
    fdsocket = socket(AF_INET,SOCK_STREAM,0);

    //VERIFICACIÓN DE SOCKET
    if(fdsocket==-1){
      puts("Hubo un error al abrir el socket");
      return ERR_SOC;
    }
    puts("[Server] -> Socket iniciado correctamente");

    //ASIGNACIÓN DE DATOS DEL SERVIDOR
    socket_info.sin_family=AF_INET;
    socket_info.sin_port=htons(50001);
    socket_info.sin_addr.s_addr=inet_addr(DIRECCION_IP_PROPIA);

    //BINDEO DE SOCKET
    if(bind(fdsocket,(struct sockaddr*) &socket_info,sizeof(struct sockaddr_in))==-1){
      puts("Hubo un error al realizar el bind");
      return ERR_BIND;
    }
    puts("[Server] -> Bind realizado correctamente");

    //CONFIGURACIÓN DEL SOCKET
    setsockopt(fdsocket, SOL_SOCKET, SO_REUSEADDR,(const void*) &valor,sizeof(int));

    //ASGINACIÓN DE LA MEMORIA DINÁMICA
    socketsLibres   = cantHilos;
    hilos           = malloc(cantHilos * sizeof(pthread_t));
    socketsClientes = calloc(cantHilos,  sizeof(int));
    estadosHilos    = malloc(cantHilos * sizeof(pthread_mutex_t));
    socketsEspera   = calloc(cantEspera, sizeof(int));

    //VERIFICA LA CORRECTA INICIALIZACIÓN DE LA MEMORIA
    if(estadosHilos == NULL || socketsClientes == NULL || hilos == NULL || socketsEspera == NULL){
      free(hilos);
      free(socketsClientes);
      free(estadosHilos);
      free(socketsEspera);
      puts("Hubo un error en reservar memoria");
      return ERR_MEM;
    }

    //CREACIÓN DE HILOS
    //INICIALIZACIÓN DE SEMÁFOROS
    for(i=0;i<cantHilos;i++){
      pthread_mutex_init((estadosHilos+i),NULL); //INICIALIZA EL SEMÁFORO DEL HILO PARA SINCRONIZACIÓN
      if(pthread_mutex_lock((estadosHilos+i))){  //PONE LOS SEMÁFOROS EN UN VALOR INICIAL DE 0
        printf("Hubo un error en la creación del semaforo %d", i);
        return ERR_MUTX;
      }
    }

    //INICIALIZA EL HILOS
    i=-1; //PARA QUE VAYAN DE 0 A N-1 Y COINCIDAN CON EL VECTOR DE SEMÁFOROS
    //BUCLE QUE CREA LOS HILOS
    while(i<(cantHilos-1)){
      pthread_mutex_lock(&semCreaT);//BLOQUEA EL INICIO DEL HILO
      if(pthread_create((hilos+i),NULL,hilo_socket,(void*)&i) < 0){ //CREA EL HILO PASANDOLE EL INDICE COMO ID DEL HILO
        printf("Hubo un error en la creación del hilo %d", i);
        return ERR_THRD;
      }
      i++;
    }

    //ESCUCHAR PUERTO
    if(listen(fdsocket,cantEspera)!=0){
      puts("[Server] -> Ocurrio un error al escuchar por el socket");
      return ERR_LIST;
    }
    puts("[Server] -> A la espera de un nuevo cliente");

    //ESPERA DE CLIENTES
    //BUCLE PRINCIPAL DEL SERVIDOR
    while(estadoServidor){
      //ACEPTA UNA CONEXIÓN ENTRANTE, ES BLOQUEANTE
      auxAccept = accept(fdsocket,NULL,NULL);

      //SI HAY SOCKETS LIBRES DESPIERTA UN HILO PARA QUE ATIENDA LA CONEXIÓN
      if(socketsLibres>0){
        //BUSCA UN SOCKET LIBRE Y GUARDA LA POSICIÓN
        i = buscarSocketLibre(socketsClientes, cantHilos);

        //HAY ERROR LÓGICO SI HAY SOCKETS LIBRES Y NO ENCUENTRA ESPACIO EN EL VECTOR DE SOCKETS
        if(i == -1){
          puts("ERROR LÓGICO");
          return ERR_SOC;
        }

        //GUARDA EL SOCKET DE LA CONEXIÓN EN EL ESPACIO LIBRE DEL VECTOR DE SOCKETS
        *(socketsClientes+i) = auxAccept;

        //PIDE EL SEMAFORO DE SOCKET LIBRE,
        pthread_mutex_lock(&semSocketLibre);
          pthread_mutex_unlock(estadosHilos+i); //DESPIERTA AL HILO QUE ATENDERÁ EL SOCKET
          socketsLibres--; //REDUCE LA CANTIDAD DE SOCKETS LIBRE
        pthread_mutex_unlock(&semSocketLibre); //DEVUELVE EL SEMÁFORO
      }
      //SI NO HAY SOCKETS LIBRES PERO SI ESPACIO EN SOCKETS DE ESPERA, GUARDA LA CONEXIÓN EN EL VECTOR DE ESPERA
      else if(sockEspera<cantEspera){
        *(socketsEspera+sockEspera) = auxAccept;//GUARDA LA CONEXIÓN EN UN ESPACIO LIBRE DEL SOCKET DE ESPERA
        send(auxAccept,(void*)msg_esp,sizeof(msg_esp),0); //MENSAJE DE SOCKETS LLENOS
        send(auxAccept,(void*)"1",sizeof(msg_esp),0);     //MENSAJE DE ESTADO DEL SERVIDOR
        sockEspera++;//AUMENTA LA CANTIDAD DE SOCKETS EN ESPERA
      }
      //SI NO HAY ESPACIO EN LOS SOCKETS O EN LA ESPERA CIERRA LA CONEXIÓN
      else{
        close(auxAccept);
      }
    }

    //UNA VEZ CERRADO EL SERVIDOR
    //ESPERA A QUE TODOS LOS HILOS TERMINEN
    for(i=0;i<cantHilos;i++)
      pthread_join(*(hilos+i),NULL);

    //CIERRE Y LIBERACIÓN DE RECURSOS
    //CIERRE SOCKET
    close(fdsocket);
    //CIERRE ARCHIVO
    //fclose(archivoRegistros);
    //LIBERACIÓN DE MEMORIA DINÁMICA
    free(hilos);
    free(socketsClientes);
    free(estadosHilos);
    //LIBERACIÓN DE SEMÁFOROS
    pthread_mutex_destroy(&semSocketLibre);
    pthread_mutex_destroy(&semArchUso);
    pthread_mutex_destroy(&semCreaT);
    for(i=0;i<cantHilos;i++)
      pthread_mutex_destroy(estadosHilos+i);

    return 0;
}

//FUNCIÓN DE HILO QUE ATIENDE UNA CONEXIÓN
void* hilo_socket(void* idHilo){
  //DECLARACIÓN DE VARIABLES
  int id = *((int*)idHilo);
  pthread_mutex_unlock(&semCreaT); //AVISA QUE FUÉ CREADO Y QUE TOMÓ UN ID
  int socket, aux, i, estadoAtencion=1, cantMensajes;
  char msgfin[TAM_MSG*30];
  char msg[TAM_MSG];

  printf("[Server] -> Hilo %d Creado\n", id);

  //BUCLE PRINCIPAL MIENTRAS EL SERVIDOR ESTÉ ACTIVO
  while(estadoServidor){
    //ESPERA SER DESPERTADO
    pthread_mutex_lock(estadosHilos+id);

    //OBTIENE EL SOCKET
    socket = *(socketsClientes+id);
    printf("[Server] -> Hilo despertado: %d\tID socket: %d\n",id,socket);

    //MENSAJE AL CLIENTE DE QUE SE CONECTÓ EL SERVIDOR
    sprintf(msg,"[Server] -> Servidor conectado");
    if(send(socket, (void*) &msg, sizeof(msg),0) == -1)
          puts("FALLO");
    sprintf(msg,"2");
    if(send(socket, (void*) &msg, sizeof(msg),0) == -1)
          puts("FALLO");

    do{
      //RECIBE MENSAJE
      if(read(socket, (void*) &msg, sizeof(msg)) > 0){
        printf("[HILO %d] -> Mensaje leido\n", id);
        printf("MENSAJE: %s", msg);

        //ALMACENA PARA MENSAJE FINAL
        strcat(msgfin,"Mensaje: ");
        strcat(msgfin,msg);
        printf("%s",msgfin);

        //RESPONDE MENSAJE
        cantMensajes = 1; //INDICA LA CANTIDAD DE MENSAJES A ENVIAR
        sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        if(send(socket, (void*) &msg, sizeof(msg),0) == -1) //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
            break;
        //WHILE PARA MANDAR TODOS LOS MENSAJES PENDIENTES
        while(cantMensajes--){
          sprintf(msg,"[HILO %d] -> Respuesta: RECIBIDO", id);
          if(send(socket, (void*) &msg, sizeof(msg),0) == -1)
            break;
        }
      }
      //SI EL CLIENTE CERRÓ EJECUTA LO SIGUIENTE
      else{
        //AVISA DE LA FINALIZACIÓN DEL CLIENTE Y MUESTRA LOS MENSAJES QUE MANDÓ EL MISMO
        printf("[HILO %d] -> Cliente finalizado\n", id);
        printf("[Server] -> ESTOS FUERON SUS MENSAJES:\n%s", msgfin);
        msgfin[0] = '\0'; //RESETEO DE MENSAJES ENVIADOS POR EL CLIENTE
        close(socket);    //CIERRE DE SOCKET
        //SI HAY UN SOCKET EN ESPERA LO ATIENDE
        if(sockEspera>0){
          socket = *(socketsEspera);
          sockEspera--;
          for(i=0;i<sockEspera;i++){
            aux = *socketsEspera;
            *socketsEspera = *(socketsEspera+1);
            *(socketsEspera+1) = aux;
          }
          sprintf(msg,"[Server] -> Nuevo cliente");
          if(send(socket, (void*) &msg, sizeof(msg),0) == -1)
                puts("FALLO");

          sprintf(msg,"1");
          if(send(socket, (void*) &msg, sizeof(msg),0) == -1)
                puts("FALLO");
        }
        //SI NO HAY SOCKETS EN ESPERA, LO AVISA, ROMPE EL WHILE, SE BLOQUE Y ESPERA A SER DESPERTADO OTRA VEZ
        else{
          puts("[Server] -> No hay sockets que atender");
          estadoAtencion = 0;
        }
      }
    }while(estadoAtencion); //BUCLE MIENTRAS HAYA CLIENTES QUE ATENDER

    //REALMENTE NO SE FINALIZA EL HILO SINO QUE PASA EL HILO A ESTADO DURMIENDO
    printf("[HILO %d] -> Finalizando hilo\n", id);
    //PIDE EL SEMÁFORO DE SOCKET LIBRE
    pthread_mutex_lock(&semSocketLibre);
      socketsLibres++;//AUMENTA LA CANTIDAD DE SOCKETS LIBRES
      *(socketsClientes+id) = 0;//"LIBERA" UN ESPACIO DEL VECTOR DE SOCKETS
    pthread_mutex_unlock(&semSocketLibre);
  }

  //SI EL SERVIDOR FINALIZÓ, FINALIZA EL HILO Y LIBERA LOS RECURSOS
  printf("[HILO %d] -> Fin hilo\n", id);
  pthread_exit(NULL);
}

int buscarSocketLibre(int* sockets, int tam){
  int i=0, encontrado=0, pos=-1;

  while(!encontrado && i<tam){
    if(*(sockets+i) == 0){
      encontrado = 1;
      pos = i;
    }
    i++;
  }

  return pos;
}
