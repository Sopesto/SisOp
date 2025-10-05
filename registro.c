#include "registro.h"

Registro* buscar_registro(Registro* buf, int tam, int op){
  Registro* ret = NULL;

  if(op==REG_COM){

  }
  else if(op==REG_VAC){


  }
  else
    puts("Operación inválida");


  return ret;
}

int escribir_registro(FILE** archivo, Registro registro){
  return 1;
}

int leer_registro(FILE** archivo, Registro* registro){
  registro=NULL;
  return 1;
}

int abrir_archivo(FILE** archivo, char* ruta, char* modo){
  return 1;
}

int verifParams(char* argvec[], int argcant, int cantParam, int(*verif)(int), char* msg){
  int i, j;
  //EXTRACCIÓN DE PARÁMETROS
  if(argcant<cantParam){
    puts("La cantidad de argumentos es insuficiente.");
    puts(msg);
    return ERR_ARG;
  }
  if(argcant>cantParam){
    puts("La cantidad de argumentos es mayor a la requerida.");
    puts(msg);
    return ERR_ARG;
  }

  //COMPROBAR QUE LOS PARÁMETROS SEAN NUMEROS
  for(i=1;i < cantParam;i++){
    for(j=0;(argvec[i])[j] != '\0';j++){
      if(verif((argvec[i])[j])){
        puts("Los argumentos son del tipo incorrecto.");
        puts(msg);
        return ERR_ARG;
      }
    }
  }

  return 1;
}

void manejadorInterrupciones(int invar){

}

void limpiarSalto(char* msg){
  char* auxmsg;
  auxmsg = msg;
  while(*(auxmsg++) != '\n');
  *auxmsg = '\0';
}
//LIMPIA EL SALTO DE LINEA

