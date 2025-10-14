#include <sys/file.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <pthread.h>
#include "registro.h"

//VARIABLES GLOBALES Y SEMÁFOROS
//archivo_en_uso se usa para avisar al cliente que el archivo está en uso sin bloquear al usuario
int   archivo_en_uso = 0, socketsLibres, sockEspera=0, estadoServidor=1, cantHilos, fdsocket;
int*  socketsClientes, *socketsEspera;
pthread_mutex_t     semSocketLibre = PTHREAD_MUTEX_INITIALIZER, semArchUso = PTHREAD_MUTEX_INITIALIZER, semCreaT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t*    estadosHilos;
pthread_t*          hilos;
FILE*               archivoRegistros;
char*               ip;

void  liberarRecursos();
void* hilo_socket(void*);
int   buscarSocketLibre(int*,int);

int main(int argc, char* argv[]){
    //CREACIÓN DE VARIABLES
    int                 auxAccept, valor=1, cantEspera, i, puerto;
    struct sockaddr_in  socket_info;
    char                msg_esp[]="Servidor lleno, espera en cola";
    FILE*               archivoRed;

    atexit(liberarRecursos);
    //MANEJO DE SEÑALES
    signal(SIGINT,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);

    //RESERVA ESPACIO PARA LA IP
    ip = malloc(sizeof(char)*16);

    //ABRE EL ARCHIVO DE CONFIGURACIÓN DE RED
    archivoRed = fopen("netconfig.config","rt");
    if(archivoRed==NULL)
      return ERR_SOC;

    //LEE EL ARCHIVO DE CONFIGURACIÓN DE RED
    if(fscanf(archivoRed,"%16[^:]:%d",ip,&puerto)<0)
      return ERR_SOC;

    fclose(archivoRed); //CIERRA EL ARCHIVO DE CONFIGURACIÓN DE RED

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

    //ABRIR ARCHIVO
    if(abrir_archivo(&archivoRegistros,NOMBRE_ARCHIVO,"r+")==-1){
      puts("Error al abrir el archivo de registros");
      return ERR_ARCH;
    }

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
    socket_info.sin_port=htons(puerto);
    socket_info.sin_addr.s_addr=inet_addr(ip);

    //BINDEO DE SOCKET
    if(bind(fdsocket,(struct sockaddr*) &socket_info,sizeof(struct sockaddr_in))==-1){
      puts("Hubo un error al realizar el bind");
      return ERR_BIND;
    }
    puts("[Server] -> Bind realizado correctamente");
    printf("[Server] -> Servidor abierto en la IP: %s\n", ip);
    printf("[Server] -> Servidor abierto en el puerto: %d\n", puerto);

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

    return 0;
}

void liberarRecursos(){
  //CIERRE Y LIBERACIÓN DE RECURSOS
  //CIERRE SOCKET
  close(fdsocket);
  //CIERRE ARCHIVO
  if(archivoRegistros!=NULL)
    fclose(archivoRegistros);
  //LIBERACIÓN DE MEMORIA DINÁMICA
  free(hilos);
  free(socketsClientes);
  free(estadosHilos);
  free(socketsEspera);
  free(ip);
  //LIBERACIÓN DE SEMÁFOROS
  pthread_mutex_destroy(&semSocketLibre);
  pthread_mutex_destroy(&semArchUso);
  pthread_mutex_destroy(&semCreaT);
  for(int i=0;i<cantHilos;i++)
    pthread_mutex_destroy(estadosHilos+i);
}

//FUNCIÓN DE HILO QUE ATIENDE UNA CONEXIÓN
void* hilo_socket(void* idHilo){
  //DECLARACIÓN DE VARIABLES
  int id = *((int*)idHilo), enTransaccion=0, errTransaccion=0, finCli;
  pthread_mutex_unlock(&semCreaT); //AVISA QUE FUÉ CREADO Y QUE TOMÓ UN ID
  int socket, aux, i, estadoAtencion=1, cantMensajes;
  char msgfin[TAM_MSG*50];
  char msg[TAM_MSG], msgErr[TAM_MSG];
  char* pmsgErr;
  FILE *archivoTemporal;

  pmsgErr = msgErr;

  printf("[Servidor] -> Hilo %d Creado\n", id);

  //BUCLE PRINCIPAL MIENTRAS EL SERVIDOR ESTÉ ACTIVO
  while(estadoServidor){
    //ESPERA SER DESPERTADO
    pthread_mutex_lock(estadosHilos+id);
    estadoAtencion = 1;
    //OBTIENE EL SOCKET
    socket = *(socketsClientes+id);
    printf("[Servidor] -> Hilo despertado: %d ID socket: %d\n",id,socket);

    //MENSAJE AL CLIENTE DE QUE SE CONECTÓ EL SERVIDOR
    sprintf(msg,"[Servidor] -> Servidor conectado");
    send(socket, (void*) &msg, sizeof(msg),0);
    sprintf(msg,"2");
    send(socket, (void*) &msg, sizeof(msg),0);

    cantMensajes = 1; //INDICA LA CANTIDAD DE MENSAJES A ENVIAR
    sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
    send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

    //MENSAJE DE MENU
    sprintf(msg,MENU_CLIENTE);
    send(socket, (void*) &msg, sizeof(msg),0);

    finCli=0;

    do{
      //RECIBE MENSAJE
      if(read(socket, (void*) &msg, sizeof(msg)) > 0 && !finCli){
        printf("[HILO %d] -> Mensaje leido\n", id);
        printf("MENSAJE: %s\n", msg);

        //ALMACENA PARA MENSAJE FINAL
        strcat(msgfin,"Mensaje: ");
        strcat(msgfin,msg);
        strcat(msgfin,"\n");

        cantMensajes = 1; //INDICA LA CANTIDAD DE MENSAJES A ENVIAR SIEMPRE MANDA UNO, EL MENÚ

        if(!strcmp(msg,"SALIR")){
          finCli=1;
          cantMensajes=0;
        }

        //RESPONDE MENSAJE
        if(!strcmp(msg,"BEGIN TRANSACTION")){
          pthread_mutex_lock(&semArchUso);
          if(!archivo_en_uso){
            archivo_en_uso=1;
            pthread_mutex_unlock(&semArchUso);
            enTransaccion=1;
            errTransaccion=0;

            sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
            send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

            sprintf(msg,"[Servidor] -> Iniciando Transacción."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
            send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

            archivoTemporal=fopen(NOMBRE_ARCHIVO_TEMPORAL,"wb+");

            if(archivoTemporal==NULL){
              puts("Error al abrir el archivo temporal");
              enTransaccion=0;
            }

            flock(fileno(archivoRegistros), LOCK_EX);

            if(!copiar_archivoTAB(archivoRegistros,archivoTemporal)){
              puts("Error al copiar los registros al archivo temporal");
              fclose(archivoTemporal);
              remove(NOMBRE_ARCHIVO_TEMPORAL);
              errTransaccion=1;
              enTransaccion=0;
            }

            while(!errTransaccion && enTransaccion){
              if(read(socket, (void*) &msg, sizeof(msg)) > 0){
                printf("[HILO %d] -> Mensaje leido\n", id);
                printf("MENSAJE: %s\n", msg);

                //ALMACENA PARA MENSAJE FINAL
                strcat(msgfin,"Mensaje: ");
                strcat(msgfin,msg);
                strcat(msgfin,"\n");

                if(!strcmp(msg,"COMMIT TRANSACTION"))
                  enTransaccion=0;

                if(enTransaccion && procesar_consulta(msg,&archivoTemporal,socket,&pmsgErr)<0){
                  puts("Error al procesar la consulta");
                  sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
                  send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

                  send(socket, (void*) &msgErr, sizeof(msgErr),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
                }

              }
              else{
                errTransaccion=1;
                enTransaccion=0;
              }

            }

            fflush(stdout);

            if(errTransaccion){
              puts("El usuario no guardó las transacciones, por lo que no se guardarán los cambios");
            }
            else{
              if(freopen(NULL,"w+",archivoRegistros)==NULL){
                puts("Error al re abrir el archivo de registros");
                errTransaccion = 1;
              }


              if(copiar_archivoBAT(archivoTemporal,archivoRegistros)<0){
                puts("Error al copiar los registros al archivo de registros");
                errTransaccion = 1;
              }
            }


            flock(fileno(archivoRegistros), LOCK_UN);

            if(archivoTemporal!=NULL){
              fclose(archivoTemporal);
              remove(NOMBRE_ARCHIVO_TEMPORAL);
            }

            sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
            send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

            if(errTransaccion){
              sprintf(msg,"[Servidor] -> Error en la transacción, abortado.\n"); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
              send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
            }
            else{
              sprintf(msg,"[Servidor] -> Fin Transacción."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
              send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
            }

            archivo_en_uso=0;
          }
          else{
            pthread_mutex_unlock(&semArchUso);
            sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
            send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

            sprintf(msg,"[Servidor] -> Respuesta: El archivo de registros está en uso. Intentelo más tarde.");
            send(socket, (void*) &msg, sizeof(msg),0);
          }
        }
        else{
          sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
          send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

          if(!finCli){
            //MENSAJE DE MENU
            sprintf(msg,MENU_CLIENTE);
            send(socket, (void*) &msg, sizeof(msg),0);
          }
        }

      }
      //SI EL CLIENTE CERRÓ EJECUTA LO SIGUIENTE
      else{
        //AVISA DE LA FINALIZACIÓN DEL CLIENTE Y MUESTRA LOS MENSAJES QUE MANDÓ EL MISMO
        printf("[HILO %d] -> Cliente finalizado\n", id);
        printf("[Servidor] -> ESTOS FUERON SUS MENSAJES:\n%s", msgfin);
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
          sprintf(msg,"[Servidor] -> Nuevo cliente");
          send(socket, (void*) &msg, sizeof(msg),0);

          sprintf(msg,"1");
          send(socket, (void*) &msg, sizeof(msg),0);
        }
        //SI NO HAY SOCKETS EN ESPERA, LO AVISA, ROMPE EL WHILE, SE BLOQUE Y ESPERA A SER DESPERTADO OTRA VEZ
        else{
          puts("[Servidor] -> No hay sockets que atender");
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
