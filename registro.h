#ifndef REGISTRO_H_INCLUDED
#define REGISTRO_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "palabra.h"

#define ERR_MEM   -1
#define ERR_SOC   -2
#define ERR_BIND  -3
#define ERR_LIST  -4
#define ERR_THRD  -5
#define ERR_MUTX  -6
#define ERR_ARCH  -7
#define ERR_SEM   -8
#define ERR_CPH   -9
#define ERR_ARG   -10
#define FIN_EPH 1

#define REG_VAC 0
#define REG_COM 1

#define TAM_MSG 256

#define NOMBRE_ARCHIVO "registros.csv"

typedef struct{
  int id;
  int productor_idx;  // índice del productor que creó el registro
  char nombre[64];    // ejemplo: nombre de medicamento
  int stock;          // cantidad en stock
  double precio;      // precio
}Registro; //ESTRUCTURA DE UN REGISTRO

//DADO UN PUNTERO A UN REGISTRO, SU TAMAÑO Y EL TIPO DE REGISTRO A BUSCAR
//DEVUELVE UN PUNTERO A UN REGISTRO SI LO ENCONTRÓ, UN PUNTERO A NULL SI NO LO HIZO
Registro* buscar_registro(Registro*,int,int);

//DADO UN ARCHIVO Y UN REGISTRO, ESCRIBE ESE REGISTRO AL FINAL DEL ARCHIVO
//DEVUELVE 1 SI PUDO ESCRIBIR, -1 SI NO PUDO.
int       escribir_registro(FILE*,Registro*);

//DADO UN ARCHIVO Y UN PUNTERO A UN REGISTRO, ESCRIBE EL REGISTRO LEÍDO EN EL PUNTERO
//DEVUELVE 1 SI PUDO LEER, -1 SI NO PUDO
int       leer_registro(FILE*,Registro*);

//DADO PUNTERO DOBLE UN ARCHIVO, UNA RUTA DE ARCHIVO Y UN MODO DE APERTURA, ABRE UN ARCHIVO Y GUARDA LA DIRECCIÓN EN EL PUNTERO SIMPLE AL ARCHIVO
//DEVUELVE 1 SI PUDO LEER, -1 SI NO PUDO
int       abrir_archivo(FILE**,const char*,const char*);

//DADO UN VECTOR DE ARGUMENTOS, CANTIDAD DE ARGUMENTOS, CANTIDAD LÍMITE DE ARGUMENTOS, UNA FUNCIÓN DE COMPROBACIÓN DE INT QUE NO DEBEN CUMPLIR LOS PARÁMETROS Y UN MENSAJE DE ERROR
//DEVUELVE 1 SI VERIFICA LOS REQUISITOS BRINDADOS, SI NO ERR_ARG
int       verifParams(char*[],int,int,int(*)(int),char*);

//LIMPIAR SALTO DE LINEA
void limpiarSalto(char*);

//HACER FUNCIONES PARA REALIZAR CONSULTAS
//LAS DE MODIFICACIÓN SE HACEN SOBRE EL ARCHIVO
//LAS DE CONSULTAS SE HACE SOBRE UN VECTOR DE REGISTROS OBTENIDO DE LEER TODOS LOS REGISTROS DEL ARCHIVO

//HACER UNA FUNCIÓN PARA QUE DADA UNA SECUENCIA DE PALABRAS LA DESCOMPONGA
//EN UNA CONSULTA Y LLAMA LA FUNCIÓN CORRESPONDIENTE PARA ATENDERLA

Registro generar_registro_aleatorio(int,int);

#endif // REGISTRO_H_INCLUDED
