#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "productor.c"

int main(int argc, char* argv[]){
    //DECLARACIÓN DE VARIABLES Y PUNTEROS
    sem_t*      semIDs, *semBuf, *semMsgBuf, *semEspBuf, *semArchAux; //SE PUEDE REEMPLAZAR EL SEMÁFORO DE ARCHIVO POR LA FUNCIÓN flock()
    pid_t*      procesos;
    Registro*   buf, registro, *posReg;
    int         shmflg, shmbuf, shmid, cantProcs, cantIds, cantCons, i, *ids;
    FILE*       archivoRegistro;

    if(verifParams(argv,argc,3,isalpha,"Argumentos: 1-Cantidad de procesos generadores 2-Cantidad total de registros.") == ERR_ARG)
      return ERR_ARG;

    //CONVERSIÓN DE TEXTO A NÚMERO DE LOS PARÁMETROS
    cantProcs = strtol(argv[1],NULL,10);
    cantIds   = strtol(argv[2],NULL,10);

    //COMPROBACIÓN DE RANGO
    if(cantProcs<=0 || cantIds<=0){
      puts("Los argumentos deben ser números positivos mayores a 0.");
      puts("Argumentos: 1-Cantidad de procesos generadores 2-Cantidad total de registros.");
      return ERR_ARG;
    }

    //ABRIR ARCHIVO
    if(abrir_archivo(&archivoRegistro,"","")==-1){
      puts("Error al abrir el archivo de registros");
      return ERR_ARCH;
    }

    //DECLARACIÓN DE SEMÁFOROS
    semIDs      = sem_open("ID",O_CREAT|O_EXCL,644,1);
    semBuf      = sem_open("BR",O_CREAT|O_EXCL,644,1);
    semMsgBuf   = sem_open("MB",O_CREAT|O_EXCL,644,0);
    semEspBuf   = sem_open("EB",O_CREAT|O_EXCL,644,ELEMENTOS_BUF);
    semArchAux  = sem_open("AU",O_CREAT|O_EXCL,644,1);

    //COMPROBACIÓN DE SEMÁFORO
    if(semIDs==SEM_FAILED || semBuf==SEM_FAILED || semMsgBuf==SEM_FAILED || semEspBuf==SEM_FAILED || semArchAux==SEM_FAILED){
      puts("Error en la creación de los semáforos");
      fclose(archivoRegistro);
      sem_unlink("ID");
      sem_unlink("BR");
      sem_unlink("MB");
      sem_unlink("EB");
      sem_unlink("AU");
      sem_close(semEspBuf);
      sem_close(semMsgBuf);
      sem_close(semBuf);
      sem_close(semIDs);
      sem_close(semArchAux);
      return ERR_SEM;
    }

    //DECLARACIÓN DE MEMORIA COMPARTIDA
    //ASIGNACIÓN DE PARÁMETROS DE LA MEMORIA COMPARTIDA
    shmflg  = IPC_CREAT | 0666;

    //DECLARACIÓN Y COMPROBACIÓN DE MEMORIA COMPARTIDA, BUFFER E IDS
    if((shmbuf = shmget(LLAVE_BUF, ELEMENTOS_BUF*sizeof(Registro), shmflg))<0 || (shmid = shmget(LLAVE_IDS, sizeof(int), shmflg))<0){
      puts("Error en la creación de la memoria compartida");
      fclose(archivoRegistro);
      sem_unlink("ID");
      sem_unlink("BR");
      sem_unlink("MB");
      sem_unlink("EB");
      sem_unlink("AU");
      sem_close(semEspBuf);
      sem_close(semMsgBuf);
      sem_close(semBuf);
      sem_close(semIDs);
      sem_close(semArchAux);
      return ERR_MEM;
    }

    //CREACIÓN DE PROCESOS HIJOS
    //ASIGNACIÓN DE ESPACIO PARA EL VECTOR DE PROCESOS
    procesos = malloc(cantProcs * sizeof(pid_t));
    //LÓGICA DE CREACIÓN DE PROCESOS
    //PIDE EL SEMÁFORO DE IDS PARA QUE NINGÚN PROCESO PRODUCTOR EMPIECE ANTES
    //DE QUE SE CREEN TODOS LOS PRODUCTORES
    sem_wait(semIDs);
    i = 0;
    while(i<cantProcs){
      *(procesos+i) = fork(); //CREA EL PROCESO HIJO Y GUARDA SU ID EN EL VECTOR DE PROCESOS

      if(*(procesos+i)<0){
        printf("Error al crear el proceso hijo %d\n",i);
        fclose(archivoRegistro);
        sem_unlink("ID");
        sem_unlink("BR");
        sem_unlink("MB");
        sem_unlink("EB");
        sem_unlink("AU");
        sem_close(semEspBuf);
        sem_close(semMsgBuf);
        sem_close(semBuf);
        sem_close(semIDs);
        sem_close(semArchAux);
        free(procesos);
        return ERR_CPH;
      }
      //SI ES UN PROCESO PRODUCTOR LLAMA A LA FUNCIÓN PRODUCTOR() Y LUEGO TERMINA
      else if(*(procesos+i)==0){
        if(productor(shmbuf,shmid,i)>0)
          return FIN_EPH;
        else
          return ERR_CPH;
      }
      i++;
    }

    //LOGICA
    //ASIGNACIÓN DE MEMORIA COMPARTIDA
    if((buf = shmat(shmbuf, NULL, 0)) == (Registro*)-1 || (ids = shmat(shmid, NULL, 0)) == (int*)-1){
      puts("Error en la asignación de la memoria compartida");
      fclose(archivoRegistro);
      shmdt(buf);
      shmdt(ids);
      shmctl(shmbuf, IPC_RMID, NULL);
      shmctl(shmid, IPC_RMID, NULL);
      sem_unlink("ID");
      sem_unlink("BR");
      sem_unlink("MB");
      sem_unlink("EB");
      sem_unlink("AU");
      sem_close(semEspBuf);
      sem_close(semMsgBuf);
      sem_close(semBuf);
      sem_close(semIDs);
      sem_close(semArchAux);
      free(procesos);
      return ERR_MEM;
    }

    *ids = cantIds;   //GUARDA LA CANTIDAD DE IDS EN LA MEMORIA COMPARTIDA
    sem_post(semIDs); //DEVUELVE EL SEMÁFORO

    //BUCLE DE LECTURA DE REGISTROS, NO TERMINA HASTA LEER TODOS LOS REGISTROS CREADOS
    while(cantCons==cantIds){
      //PIDE MENSAJE Y LUEGO BUFFER
      sem_wait(semMsgBuf);
      sem_wait(semBuf);
      //BUSCA UN REGISTRO PARA ESCRIBIR EN EL ARCHIVO
      posReg = buscar_registro(buf,ELEMENTOS_BUF,REG_COM);
      if(posReg==NULL){
        puts("No se encontró ningún registro para leer");
        sem_post(semMsgBuf);
      }
      else{
        registro   = *posReg;
        posReg->id = 0;
      }
      //DEVUELVE EL BUFFER Y AUMENTA EL ESPACIO DEL BUFFER
      sem_post(semBuf);
      sem_post(semEspBuf);
      puts("Proceso consumidor escribió en el buffer.");

      //ESCRIBE EL REGISTRO DEL BUFFER EN EL ARCHIVO
      if(registro.id!=-1){
        if(escribir_registro(&archivoRegistro, registro)==-1)
          puts("Error al escribir el archivo");
        else{
          puts("Proceso consumidor escribió en el archivo de registros.");
          cantCons++; //SI TODO SALIÓ BIEN AUMENTA EL CONTADOR DE REGISTROS ESCRITOS EN 1
        }
      }
    }

    //ESPERAR A LA FINALIZACIÓN DE PROCESOS
    i = 0;
    while(i<cantProcs){
      waitpid(*(procesos+i),NULL,0);
      i++;
    }

    //CERRAR ARCHIVO
    fclose(archivoRegistro);
    //DESASIGNACIÓN DE MEMORIA COMPARTIDA
    shmdt(buf);
    shmdt(ids);
    //LIBERACIÓN DE MEMORIA COMPARTIDA
    shmctl(shmbuf, IPC_RMID, NULL);
    shmctl(shmid, IPC_RMID, NULL);
    //DESASIGNACIÓN DE NOMBRE DE LOS SEMÁFOROS
    sem_unlink("ID");
    sem_unlink("BR");
    sem_unlink("MB");
    sem_unlink("EB");
    sem_unlink("AU");
    //CIERRE DE LOS SEMÁFOROS
    sem_close(semEspBuf);
    sem_close(semMsgBuf);
    sem_close(semBuf);
    sem_close(semIDs);
    sem_close(semArchAux);
    //LIBERACIÓN DE VECTOR PROCESOS
    free(procesos);

    return 0;
}


