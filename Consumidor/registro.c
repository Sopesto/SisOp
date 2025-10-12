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

  return 1;
}

int leer_registro(FILE* archivo, Registro* registro){
  char cadena[257];
  char* aux;

  if(fgets(cadena,sizeof(cadena),archivo)==NULL)
    return 0;

  aux=strchr(cadena,'\n');
  if(aux==NULL)
    return -1;
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  sscanf(aux+1,"%lf",&registro->precio);
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  sscanf(aux+1,"%d",&registro->stock);
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  strcpy(registro->nombre,aux+1);
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  sscanf(aux+1,"%d",&registro->productor_idx);
  *aux='\0';

  sscanf(cadena,"%d",&registro->id);

  return 1;
}

int leer_registro_de_cadena(char* cadena, Registro* registro){
  char* aux;

  aux=strchr(cadena,')');
  if(aux==NULL)
    return -1;
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  sscanf(aux+1,"%lf",&registro->precio);
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  sscanf(aux+1,"%d",&registro->stock);
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  strcpy(registro->nombre,aux+1);
  *aux='\0';

  aux=strrchr(cadena,',');
  if(aux==NULL)
    return -1;
  sscanf(aux+1,"%d",&registro->productor_idx);
  *aux='\0';

  if(aux==NULL)
    return -1;
  sscanf(cadena+1,"%d",&registro->id);

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

//PROCESA UNA CONSULTA Y REALIZA LAS MODIFICACIONES CORRESPONDIENTES EN EL ARCHIVO
int procesar_consulta(char* consulta, FILE **archivo, int socket){
  char msg[TAM_MSG];
  int registros, comando=-1, campo=-1, cantMensajes = 1, i, asc, ret=-1;
  Registro reg, *regis;
  void* valor;
  int (*funcionOrdenar)(const void*, const void*);
  Palabra pcomando, pcampo, pvalor, orden;

  pcomando  = leer_palabra(&consulta);
  comando   = obtener_comando(pcomando);
  //PROBLEMA ACÁ
  if(comando<0){
    puts("Comando inválido");

    return ret;
  }

  if(comando != 0 && comando != 2){
    pcampo    = leer_palabra(&consulta);
    campo     = pos_campo(pcampo);
  }

  if(comando != 0 && comando != 5){
    pvalor    = leer_palabra(&consulta);
    if(comando==4)
      valor   = valor_campo(pvalor,-1);
    else
      valor   = valor_campo(pvalor,campo);
  }

  orden     = leer_palabra(&consulta);
  asc       = ascendencia(orden);

  switch(comando){
    case 0:
    //AYUDA
    sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
    send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

    ret = ayuda(socket);


    break;

    case 1:
    //MOSTRAR
    fseek(*archivo,0,SEEK_SET);
    fgets(msg,sizeof(msg),*archivo);
    registros = contar_registros(*archivo);

    regis = malloc(registros*sizeof(Registro));
    if(regis==NULL)
      return -1;

    i=0;

    while(fread(&reg,sizeof(Registro),1,*archivo)>0){
      if(campo>=0){
        switch(campo){
          case 0:
            if(reg.id!=*((int*)valor))
              continue;
          break;
          case 1:
            if(reg.productor_idx!=*((int*)valor))
              continue;
          break;
          case 2:
            if(strcmp(reg.nombre,((char*)valor)))
              continue;
          break;
          case 3:
            if(reg.stock!=*((int*)valor))
              continue;
          break;
          case 4:
            if(reg.id!=*((double*)valor))
              continue;
          break;
        }
      }

      *(regis+i)=reg;
      cantMensajes++;
      i++;
    }

    sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
    send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

    ret = mostrar(regis,cantMensajes,socket);

    free(regis);


    break;

    case 2:
    //INSERTAR
    if(valor != NULL){
      sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
      send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

      if((ret = insertar(*archivo, *((Registro*) valor)))<0){
        sprintf(msg,"ERROR AL INSERTAR EL ELEMENTO."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
      else{
        sprintf(msg,"ELEMENTO INSERTADO CON ÉXITO."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
    }


    break;

    case 3:
    //ELIMINAR
    if(valor != NULL && campo != -1){
      sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
      send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

      if((ret = eliminar(archivo,campo,valor))<0){
        sprintf(msg,"ERROR AL ELIMINAR LOS ELEMENTOS."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
      else if(ret==0){
        sprintf(msg,"NO SE ENCONTRÓ EL ELEMENTO."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
      else{
        sprintf(msg,"ELEMENTOS ELIMINADOS CON ÉXITO."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
    }


    break;

    case 4:
    //ACTUALIZAR
    printf("Campo: %d",campo);
    if(valor != NULL && campo != -1){
      sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
      send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

      if((ret = actualizar(*archivo,campo,valor) )<0){
        sprintf(msg,"ERROR AL ACTUALIZAR LOS ELEMENTOS."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
      else{
        sprintf(msg,"ELEMENTOS ACTUALIZADOS CON ÉXITO."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
    }


    break;

    case 5:
    //ORDENAR
    if(asc != -1 && campo != -1){
      registros = contar_registros(*archivo);
      switch(campo){
        case 0:
          if(asc)
            funcionOrdenar = ordenarIDAsc;
          else
            funcionOrdenar = ordenarIDDesc;
        break;
        case 1:
          if(asc)
            funcionOrdenar = ordenarIDAsc;
          else
            funcionOrdenar = ordenarIDDesc;
        break;
        case 2:
          if(asc)
            funcionOrdenar = ordenarIDAsc;
          else
            funcionOrdenar = ordenarIDDesc;
        break;
        case 3:
          if(asc)
            funcionOrdenar = ordenarIDAsc;
          else
            funcionOrdenar = ordenarIDDesc;
        break;
        case 4:
          if(asc)
            funcionOrdenar = ordenarIDAsc;
          else
            funcionOrdenar = ordenarIDDesc;
        break;
      }

      sprintf(msg,"%d",cantMensajes); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
      send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN

      if((ret = ordenar(*archivo,registros,campo, funcionOrdenar))<0){
        sprintf(msg,"ERROR AL ORDENAR LOS ELEMENTOS."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
      else{
        sprintf(msg,"ELEMENTOS ORDENADOS CON ÉXITOS."); //GUARDA EN EL MENSAJE LA CANTIDAD DE MENSAJES A ENVIAR
        send(socket, (void*) &msg, sizeof(msg),0); //LE DICE AL CLIENTE CUANTOS MENSAJES SE ENVIARÁN
      }
    }

    break;
  }

  free(valor);
  return ret;
}

//CUENTA LA CANTIDAD DE REGISTROS EN UN ARCHIVO DE TEXTO
size_t contar_registros(FILE *archivo){
    size_t tam;

    fseek(archivo, 0, SEEK_END);
    tam = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    return tam/sizeof(Registro);
}

//COPIA TODOS LOS REGISTROS DE UN ARCHIVO TEXTO EN UNO BINARIO
int copiar_archivoTAB(FILE *archivoBase, FILE *archivoDestino){
  Registro reg;

  fseek(archivoBase,0,SEEK_SET);
  leer_registro(archivoBase,&reg);
  while(leer_registro(archivoBase,&reg)>0){
    if(fwrite(&reg,sizeof(reg),1,archivoDestino)<1)
      return 0;
  }
  fflush(archivoDestino);

  return 1;
}

//COPIA TODOS LOS REGISTROS DE UN ARCHIVO BINARIO EN UNO DE TEXTO
int copiar_archivoBAT(FILE *archivoBase, FILE *archivoDestino){
  Registro reg;

  fseek(archivoBase,0,SEEK_SET);
  fprintf(archivoDestino,"ID,PRODUCTOR,NOMBRE,STOCK,PRECIO\n");
  while(fread(&reg,sizeof(reg),1,archivoBase)){
    if(escribir_registro(archivoDestino, &reg)<0)
      return -1;
  }
  fflush(archivoDestino);

  return 1;
}

//MUESTRA EL MENSAJE DE AYUDA - MENU
int ayuda(int socket){
  char msg[]="menu";
  send(socket, (void*) &msg, sizeof(msg),0);
  return 1;
}

//MUESTRA LA CANTIDAD DE REGISTROS DADOS, SI NO SE ESPECIFICA (-1) MUESTRA TODOS LOS REGISTROS
int mostrar(Registro* regists, int registros, int socket){
  int i=0;
  char msg[TAM_MSG];
  Registro regis;

  //MUESTRA EL ARCHIVO AL USUARIO
  sprintf(msg,"ID\t\t ID Productor\t\t Nombre\t\t\t Stock\t\t Precio");
  send(socket, (void*) &msg, sizeof(msg),0);
  registros--;
  //WHILE PARA MANDAR TODOS LOS MENSAJES PENDIENTES
  while(registros>0){
    regis=*(regists+i);
    sprintf(msg,"%d\t\t %d\t\t\t %s\t\t %d\t\t %f", regis.id,regis.productor_idx,regis.nombre,regis.stock,regis.precio);
    send(socket, (void*) &msg, sizeof(msg),0);
    i++;
    registros--;
  }

  return 1;
}

//INSERTA UN REGISTRO AL FINAL
int insertar(FILE* archivo, Registro registro){
  int encontrado = 0, ret =-1;
  Registro reg;

  if(registro.id<0 || registro.productor_idx<0 || strlen(registro.nombre)<2 || registro.precio<0 || registro.stock<0)
    return ret;

  fseek(archivo,0,SEEK_SET);

  while(!encontrado && fread(&reg,sizeof(Registro),1,archivo)>0){
    if(reg.id == registro.id)
      encontrado = 1;
  }

  if(!encontrado){
    if(fwrite(&registro,sizeof(Registro),1,archivo)<1)
      return ret;

    fflush(archivo);
    ret = 1;
  }
  else
    puts("EL ID YA EXISTE");


  return ret;
}

//ELIMINA TODOS LOS REGISTROS DONDE EL CAMPO ELEGIDO TENGA EL VALOR ESPECIFICADO
int eliminar(FILE** archivo, int campo, void* valor){
  Registro reg;
  int encontrado=0;
  FILE* archivo_aux;
  fseek(*archivo,0,SEEK_SET);

  archivo_aux=fopen(".archAux.temp","wb+");
  if(archivo_aux==NULL)
    return -1;

  while(fread(&reg,sizeof(Registro),1,*archivo)){
    switch(campo){
      case 0:
        if(reg.id==*((int*)valor)){
          encontrado=1;
          continue;
        }
      break;
      case 1:
        if(reg.productor_idx==*((int*)valor)){
          encontrado=1;
          continue;
        }
      break;
      case 2:
        if(!strcmp(reg.nombre,((char*)valor))){
          encontrado=1;
          continue;
        }
      break;
      case 3:
        if(reg.stock==*((int*)valor)){
          encontrado=1;
          continue;
        }
      break;
      case 4:
        if(reg.id==*((double*)valor)){
          encontrado=1;
          continue;
        }
      break;
    }

    fwrite(&reg,sizeof(Registro),1,archivo_aux);
  }

  fflush(archivo_aux);
  fclose(*archivo);
  remove(NOMBRE_ARCHIVO_TEMPORAL);
  rename(".archAux.temp",NOMBRE_ARCHIVO_TEMPORAL);
  *archivo = archivo_aux;

  return encontrado;
}

int actualizar(FILE* archivo, int campo, void* valor){
  Registro reg, registroa;
  int encontrado;

  fseek(archivo,0,SEEK_SET);

  registroa = *(Registro*) valor;

  fread(&reg,sizeof(Registro),1,archivo);
  while(!feof(archivo)){
    encontrado = 1;
    switch(campo){
      case 0:
        if(reg.id!= registroa.id)
          encontrado = 0;
      break;
      case 1:
        if(reg.productor_idx!=registroa.productor_idx)
          encontrado = 0;
      break;
      case 2:
        if(strcmp(reg.nombre,registroa.nombre))
          encontrado = 0;
      break;
      case 3:
        if(reg.stock!=registroa.stock)
          encontrado = 0;
      break;
      case 4:
        if(reg.id!=registroa.precio)
          encontrado = 0;
      break;
    }

    if(encontrado){
      if(registroa.id<0)
      registroa.id=reg.id;
      else if(registroa.id==0)
        return -1;

      if(registroa.productor_idx<0)
        registroa.productor_idx=reg.productor_idx;

      if(registroa.nombre[0]=='-')
        strcpy(registroa.nombre,reg.nombre);

      if(registroa.stock<=0)
        registroa.stock=reg.stock;

      if(registroa.precio<=0)
        registroa.precio=reg.precio;
      else if(registroa.precio==0)
        return -1;

      fseek(archivo,-1*sizeof(Registro),SEEK_CUR);
      fwrite(&registroa,sizeof(Registro),1,archivo);
      fseek(archivo,0,SEEK_CUR);
      fread(&reg,sizeof(Registro),1,archivo);
    }
    else
      fread(&reg,sizeof(Registro),1,archivo);
  }

  fflush(archivo);

  return 1;
}

//ORDENA POR UNA FUNCIÓN DADA Y UN CAMPO DADO
int ordenar(FILE* archivo, int registros, int campo, int (*ord)(const void*,const void*)){
  int i=0;
  Registro* regis;
  regis = malloc(registros*sizeof(Registro));
  if(regis==NULL)
    return -1;

  while(registros>0){
    fread((regis+i),sizeof(Registro),1,archivo);
    i++;
    registros--;
  }

  qsort(regis, registros, sizeof(Registro), ord);

  fseek(archivo,0,SEEK_SET);

  while(i>0){
    fwrite((regis+i-1),sizeof(Registro),1,archivo);
    i--;
  }

  return 1;
}

int obtener_comando(Palabra pcomando){
  char comando[64];
  int idCom = -1;

  strncpy(comando,pcomando.inicio,pcomando.tam);
  comando[pcomando.tam] = '\0';

  if(!strcmp(comando,"AYUDA"))
    idCom = 0;
  else if(!strcmp(comando,"MOSTRAR"))
    idCom = 1;
  else if(!strcmp(comando,"INSERTAR"))
    idCom = 2;
  else if(!strcmp(comando,"ELIMINAR"))
    idCom = 3;
  else if(!strcmp(comando,"ACTUALIZAR"))
    idCom = 4;
  else if(!strcmp(comando,"ORDENAR"))
    idCom = 5;

  return idCom;
}

int pos_campo(Palabra pcampo){
  char campo[64];
  int idCampo = -1;

  strncpy(campo,pcampo.inicio,pcampo.tam);
  campo[pcampo.tam] = '\0';

  if(!strcmp(campo,"ID"))
    idCampo = 0;
  else if(!strcmp(campo,"PRODUCTOR"))
    idCampo = 1;
  else if(!strcmp(campo,"NOMBRE"))
    idCampo = 2;
  else if(!strcmp(campo,"STOCK"))
    idCampo = 3;
  else if(!strcmp(campo,"PRECIO"))
    idCampo = 4;

  return idCampo;
}

int ascendencia(Palabra ord){
  char ascend[64];
  int asc = -1;

  strncpy(ascend,ord.inicio,ord.tam);
  ascend[ord.tam] = '\0';

  if(!strcmp(ascend,"ASC"))
    asc = 0;
  else
    asc = 1;

  return asc;
}

void* valor_campo(Palabra valor, int campo){
  char val[64];
  void* ret=NULL;

  strncpy(val,valor.inicio,valor.tam);
  val[valor.tam] = '\0';

  switch(campo){
    case -1:
      ret = malloc(sizeof(Registro));
      if(leer_registro_de_cadena(val,((Registro*)ret))<0)
        ret = NULL;
    break;
    case 0:
      ret = malloc(sizeof(int));
      *((int*)ret) = atoi(val);
    break;
    case 1:
      ret = malloc(sizeof(int));
      *((int*)ret) = atoi(val);
    break;
    case 2:
      ret = malloc(sizeof(char)*64);
      strncpy((char*)ret,val,64);
    break;
    case 3:
      ret = malloc(sizeof(int));
      *((int*)ret) = atoi(val);
    break;
    case 4:
      ret = malloc(sizeof(double));
      *((int*)ret) = atof(val);
    break;
  }

  return ret;
}

Palabra leer_palabra(char** cadena){
  Palabra palabra;
  int i=0, salvaPar=0;


  palabra.inicio = *cadena;

  while(salvaPar || (*(*cadena+i)!='\0' && *(*cadena+i)!=' ')){
    if(*(*cadena+i)=='(')
        salvaPar++;
    if(*(*cadena+i)==')')
        salvaPar--;
    i++;
  }

  palabra.tam = i;

  if(*(*cadena+i)!='\0')
    (*cadena)++;
  *cadena = (*cadena+i);

  return palabra;
}

int ordenarIDAsc(const void* a, const void* b){
  const Registro* ra = (Registro*)a;
  const Registro* rb = (Registro*)b;

  return ra->id > rb->id;
}

int ordenarIDDesc(const void* a, const void* b){
  const Registro* ra = (Registro*)a;
  const Registro* rb = (Registro*)b;

  return ra->id > rb->id;
}
