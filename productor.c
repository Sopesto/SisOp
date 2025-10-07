#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "registro.h"

#define REGS_A_LEER 10
#define ELEMENTOS_BUF 5
#define LLAVE_BUF 1759
#define LLAVE_IDS 2005

int productor(int,int,int);

int productor(int shmbuf, int shmid, int procesoID){
  //DECLARACIÓN DE VARIABLES Y PUNTEROS
  Registro* buf,/* *registro=NULL,*/ *posReg;
  sem_t*    semIDs, *semBuf, *semMsgBuf, *semEspBuf, *semArchAux;
  int*      ids, cantIDs=0, totalIDs/*, leido*/;
  FILE*     archivoAux=NULL;

  signal(SIGINT,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);

  srand(time(NULL) ^ procesoID);

  //ABRIR ARCHIVO
  /*if(abrir_archivo(&archivoAux,"","a+")==-1){
    puts("Error al abrir el archivo de registros");
    return ERR_ARCH;
  }*/

  //DECLARACIÓN DE SEMÁFOROS
  semIDs      = sem_open("/ID",0);
  semBuf      = sem_open("/BR",0);
  semMsgBuf   = sem_open("/MB",0);
  semEspBuf   = sem_open("/EB",0);
  semArchAux  = sem_open("/AU",0);

  //COMPROBACIÓN DE SEMÁFORO
  if(semIDs==SEM_FAILED || semBuf==SEM_FAILED || semMsgBuf==SEM_FAILED || semEspBuf==SEM_FAILED || semArchAux==SEM_FAILED){
    printf("[Productor %d] -> Error en la apertura de semáforos", procesoID);
    sem_close(semEspBuf);
    sem_close(semMsgBuf);
    sem_close(semBuf);
    sem_close(semIDs);
    sem_close(semArchAux);
    fclose(archivoAux);

    return ERR_SEM;
  }

  //ASIGNACIÓN DE MEMORIA COMPARTIDA
  if((buf = shmat(shmbuf, NULL, 0)) == (Registro*)-1 || (ids = shmat(shmid, NULL, 0)) == (int*)-1){
    printf("[Productor %d] -> Error en la asignación de la memoria compartida", procesoID);
    shmdt(buf);
    shmdt(ids);
    sem_close(semEspBuf);
    sem_close(semMsgBuf);
    sem_close(semBuf);
    sem_close(semIDs);
    sem_close(semArchAux);
    fclose(archivoAux);

    return ERR_MEM;
  }

  //LOGICA
  sem_wait(semIDs);//PIDE EL SEMÁFORO DE IDS PARA LEERLO EN EL WHILE
  //EL WHILE LEE LA CANTIDAD DE IDS EN LA MEMORIA COMPARTIDA, SI ES 0 TERMINA EL WHILE
  while(*ids>0){

    //TOMA 10 IDS (O LA CANTIDAD INDICADA EN REGS_A_LEER) O HASTA QUE LOS IDS DE LA MEMORIA COMPARTIDA SEAN 0
    while(*ids>0 && cantIDs<REGS_A_LEER){
      cantIDs++;
      (*ids)--;
    }
    totalIDs = *ids; //ALMACENA LA CANTIDAD TOTAL DE IDS QUE ESTABA EN EL BUFFER PARA LUEGO ASIGNA EL ID AL REGISTRO
    sem_post(semIDs);//DEVUELVE EL SEMÁFORO DE IDS

    //LÓGICA DE CREACIÓN DE REGISTROS
    //PROCESA TODOS LOS REGISTROS PENDIENTES
    while(cantIDs>0){
      /*sem_wait(semArchAux);//PIDE EL SEMÁFORO DE ARCHIVO
        //LEE UN REGISTRO DEL ARCHIVO
        if(leer_registro(&archivoAux, registro)==-1){
          leido = 0;
          printf("[Productor %d] -> Error al leer el archivo", procesoID);
        }
        else{
          leido = 1; //INDICA QUE LEYÓ CORRECTAMENTE EL ARCHIVO
          printf("[Productor %d] -> Leyó el archivo auxiliar. \n", procesoID);
        }
      sem_post(semArchAux);//DEVUELVE EL SEMÁFORO DE ARCHIVO
      */
      if(1/*leido*/){
        sem_wait(semEspBuf); //PIDE SEMÁFORO DE ESPACIO EN BUFFER
        sem_wait(semBuf);    //PIDE SEMÁFRO DE BUFFER
          //BUSCA UN REGISTRO LIBRE EN EL BUFFER
          posReg = buscar_registro(buf,ELEMENTOS_BUF,REG_VAC);
          //SI NO ENCONTRÓ UN REGISTRO LIBRE RESETEA EL ESTADO DE PRODUCCIÓN DE REGISTRO

          if(posReg==NULL){
            printf("[Productor %d] -> No se encontró ningún espacio para escribir",procesoID);
            //leido = 0;
          }
          else{
            Registro r;
            r.id = totalIDs+cantIDs;
            r.productor_idx = procesoID;
            const char *nombres[] = {"Paracetamol","Ibuprofeno","Amoxicilina","Atorvastatina","Omeprazol","Loratadina","Cetirizina"};
            int n = sizeof(nombres)/sizeof(nombres[0]);
            const char *sel = nombres[rand() % n];
            strncpy(r.nombre, sel, sizeof(r.nombre)-1);
            r.nombre[sizeof(r.nombre)-1] = '\0';
            r.stock = (rand() % 100) + 1; // 1..100
            r.precio = ((rand() % 2000) + 100) / 100.0; // 1.00 .. 20.99
            *posReg = r;
            //*registro = generar_registro_aleatorio(totalIDs+cantIDs,procesoID);
            //registro->id = totalIDs+cantIDs; //GUARDA UN ID ÚNICO PARA EL REGISTRO CREADO
            //*posReg = *registro; //GUARDA EL REGISTRO EN LA POSICIÓN OBTENIDA
          }

        sem_post(semBuf);     //DEVUELVE SEMÁFORO DE BUFFER
        sem_post(semMsgBuf);  //DEVUELVE SEMÁFORO DE MENSAJE
      }

      //SI ESCRIBIÓ EL REGISTRO CORRECTAMENTE REDUCE EL CONTADO DE REGISTROS A ESCRIBIR
      if(1/*leido*/){
        printf("[Productor %d] -> Escribió en el buffer. \n", procesoID);
        cantIDs--;
      }
    }

    sem_wait(semIDs);//DEVUELVE EL SEMÁFORO DE IDS
  }
  sem_post(semIDs);//DEVUELVE EL SEMÁFORO DE IDS
  printf("[Productor %d] -> Finalizó\n",procesoID+1);
  //DESASIGNACIÓN DE MEMORIA COMPARTIDA
  shmdt(buf);
  shmdt(ids);
  //CIERRE DEL SEMÁFORO (REDUCCIÓN DE CONTADOR)
  sem_close(semEspBuf);
  sem_close(semMsgBuf);
  sem_close(semBuf);
  sem_close(semIDs);
  sem_close(semArchAux);
  //CIERE DE ARCHIVO AUXILIAR
  fclose(archivoAux);
  return 1;
}
