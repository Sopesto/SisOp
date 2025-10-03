#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include "registro.h"
//#include "palabra.h"

#define ERR_MEM   -1
#define ERR_SOC   -2
#define ERR_BIND  -3
#define ERR_LIST  -4
#define ERR_THRD  -5
#define ERR_MUTX  -6

#define DIRECCION_IP_SERVER "127.0.0.1"
#define DIRECCION_IP_PROPIA "127.0.0.1"

int   archivo_en_uso = 0, socketsLibres;
int*  socketsClientes;
pthread_mutex_t     semSocketLibre = PTHREAD_MUTEX_INITIALIZER, semArchUso = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t*    estadosHilos;

void* hilo_socket(void*);
int   buscarSocketLibre(int*,int);

int main(int argc, char* argv[]){
    int                 fdsocket, auxAccept, valor=1, cantHilos=5, cantConexSim=3, i, *param;
    struct sockaddr_in  socket_info;
    pthread_t*          hilos;
    FILE*               archivoRegistros;

    //EXTRAER PARÁMETROS


    //ABRIR ARCHIVO


    //ABRIR SOCKET
    fdsocket = socket(AF_INET,SOCK_STREAM,0);

    if(fdsocket==-1){
      puts("Hubo un error al abrir el socket");
      return ERR_SOC;
    }
    puts("[Server] -> Socket iniciado correctamente");

    socket_info.sin_family=AF_INET;
    socket_info.sin_port=htons(50001);
    socket_info.sin_addr.s_addr=inet_addr(DIRECCION_IP_PROPIA);

    //BINDEO DE SOCKET
    if(bind(fdsocket,(struct sockaddr*) &socket_info,sizeof(struct sockaddr_in))==-1){
      puts("Hubo un error al realizar el bind");
      return ERR_BIND;
    }
    puts("[Server] -> Bind realizado correctamente");

    setsockopt(fdsocket, SOL_SOCKET, SO_REUSEADDR,(const void*) &valor,sizeof(int));

    //INICIALIZAR HILOS
    socketsLibres   = cantHilos;
    hilos           = malloc(cantHilos * sizeof(pthread_t));
    socketsClientes = calloc(cantHilos, sizeof(int));
    estadosHilos    = malloc(cantHilos * sizeof(pthread_mutex_t));

    if(estadosHilos == NULL || socketsClientes == NULL || hilos == NULL){
      puts("Hubo un error en reservar memoria");
      return ERR_MEM;
    }

    for(i=0;i<cantHilos;i++){
      pthread_mutex_init((estadosHilos+i),NULL);
      if(pthread_mutex_lock((estadosHilos+i))){
        printf("Hubo un error en la creación del semaforo %d", i);
        return ERR_MUTX;
      }
    }

    for(i=0;i<cantHilos;i++){
      param = &i;
      sleep(1);
      if(pthread_create((hilos+i),NULL,hilo_socket,(void*)param) < 0){
        printf("Hubo un error en la creación del hilo %d", i);
        return ERR_THRD;
      }
    }

    //ESCUCHAR PUERTO
    if(listen(fdsocket,cantConexSim)!=0){
      puts("[Server] -> Ocurrio un error al escuchar por el socket");
      return ERR_LIST;
    }
    puts("[Server] -> A la espera de un nuevo cliente");

    //ESPERA DE CLIENTES
    while(1){
      while(!socketsLibres){
      }

      i = buscarSocketLibre(socketsClientes, cantHilos);

      if(i == -1){
        puts("ERROR LÓGICO");
        return ERR_SOC;
      }

      auxAccept = accept(fdsocket,NULL,NULL);
      if(socketsLibres>(cantHilos-cantConexSim)){
        *(socketsClientes+i) = auxAccept;

        pthread_mutex_lock(&semSocketLibre);
          pthread_mutex_unlock(estadosHilos+i);
          socketsLibres--;
        pthread_mutex_unlock(&semSocketLibre);
      }
      else
        close(auxAccept);

    }

    close(fdsocket);
    fclose(archivoRegistros);
    free(hilos);
    free(socketsClientes);
    free(estadosHilos);
    pthread_mutex_destroy(&semSocketLibre);
    pthread_mutex_destroy(&semArchUso);
    for(i=0;i<cantHilos;i++)
      pthread_mutex_destroy(estadosHilos+i);

    return 0;
}


void* hilo_socket(void* idHilo){
  int id = *((int*)idHilo);
  int socket;
  char msgfin[200];
  char msg[101];

  strcpy(msgfin,"Mensaje: ");

  printf("HILO %d\n", id);

  while(1){
    //ESPERA SER DESPERTADO
    pthread_mutex_lock(estadosHilos+id);

    socket = *(socketsClientes+id);
    printf("HILO DESPERTADO: %d - %d\n",id,socket);
    sprintf(msg,"[Server] -> Servidor conectado\n");
    if(write(socket, (void*) &msg, sizeof(msg)) == -1)
          puts("FALLO");
    do{
      //RECIBE MENSAJE
      if(read(socket, (void*) &msg, sizeof(msg)) != 0){
        puts("[Server] -> Mensaje leido");
        printf("MENSAJE: %s\n", msg);

        strcat(msgfin,msg);
        strcat(msgfin," ");

        //RESPONDE MENSAJE
        sprintf(msg,"[Server] -> Respuesta: HILO %d. FIN\n", id);
        if(write(socket, (void*) &msg, sizeof(msg)) == -1)
          puts("FALLO");
      }
      else{
        puts("CLIENTE FINALIZADO");
        break;
      }

    }while(1);

    printf("%s\n", msgfin);

    close(socket);

    puts("FINALIZANDO HILO");
    pthread_mutex_lock(&semSocketLibre);
      socketsLibres++;
      *(socketsClientes+id) = 0;
    pthread_mutex_unlock(&semSocketLibre);
  }

  puts("FIN HILO");
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
