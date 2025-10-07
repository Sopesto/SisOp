#include "registro.h"

Registro* buscar_registro(Registro* buf, int tam, int op){
  Registro* ret = NULL;

  if(op==REG_COM){
    for(int i=0;i<tam;i++){
      if((buf+i)->id != 0)
        ret=(buf+i);
    }
  }
  else if(op==REG_VAC){
    for(int i=0;i<tam;i++){
      if((buf+i)->id == 0)
        ret=(buf+i);
    }
  }
  else
    puts("Operación inválida");


  return ret;
}

int escribir_registro(FILE* archivo, Registro* r){
  if(!archivo || !r) return -1;
    if(fprintf(archivo, "%d,%d,%s,%d,%.2f\n", r->id, r->productor_idx, r->nombre, r->stock, r->precio) < 0)
        return -1;
    return 0;
}

int leer_registro(FILE* archivo, Registro* registro){
  char cadena[251];
  char* aux;

  fgets(cadena,sizeof(cadena),archivo);
  aux=strchr(cadena,'\n');
  *aux='\0';

  aux=strrchr(cadena,',');
  sscanf(aux+1,"%lf",&registro->precio);
  *aux='\0';

  aux=strrchr(cadena,',');
  sscanf(aux+1,"%d",&registro->stock);
  *aux='\0';

  aux=strrchr(cadena,',');
  strcpy(registro->nombre,aux+1);
  *aux='\0';

  aux=strrchr(cadena,',');
  sscanf(aux+1,"%d",&registro->productor_idx);
  *aux='\0';

  sscanf(cadena,"%d",&registro->id);

  return 1;
}

int abrir_archivo(FILE** archivo, const char* ruta, const char* modo){
  (void)ruta;
  *archivo = fopen(ruta, modo);
  if(!*archivo) return -1;
  return 0;
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

void limpiarSalto(char* msg){
  char* auxmsg;
  auxmsg = msg;
  while(*(++auxmsg) != '\n');
  *auxmsg = '\0';
}
//LIMPIA EL SALTO DE LINEA

Registro generar_registro_aleatorio(int id, int productor_idx){
    Registro r;
    r.id = id;
    r.productor_idx = productor_idx;
    const char *nombres[] = {"Paracetamol","Ibuprofeno","Amoxicilina","Atorvastatina","Omeprazol","Loratadina","Cetirizina"};
    int n = sizeof(nombres)/sizeof(nombres[0]);
    const char *sel = nombres[rand() % n];
    strncpy(r.nombre, sel, sizeof(r.nombre)-1);
    r.nombre[sizeof(r.nombre)-1] = '\0';
    r.stock = (rand() % 100) + 1; // 1..100
    r.precio = ((rand() % 2000) + 100) / 100.0; // 1.00 .. 20.99
    return r;
}
