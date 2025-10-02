#include "Palabra.h"

#define aMayus(c) ( ((c)>96 && (c)<123)?(c)-32:(c))
#define aMinus(c) ( ((c)>64 && (c)<91) ?(c)+32:(c))
#define esLetra(c) ( ((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') )

void palabraATitulo(Palabra* pal){
  char* i = pal->vPal;

  *i = aMayus(*i);
  i++;

  while(*i){ //SI LLEGA AL CARACTER NULO, QUE ES 0, TERMINA EL WHILE
    *i = aMinus(*i);
    i++;
  }
}

void secPalCrear(SecPal* sec, char* cad){
  sec->cursor = cad;
  sec->finSec = false;
}

bool secPalLeer(SecPal* sec, Palabra* pal){
  char* i = pal->vPal;

  while( (*sec->cursor !='\0') && (!esLetra(*sec->cursor)) )
    sec->cursor++;

  if((*sec->cursor =='\0')){
    sec->finSec=true;
    return false;
  }

  while(esLetra(*sec->cursor)){
    *i = *sec->cursor;
    i++;
    sec->cursor++;
  }

  *i = '\0';

  return true;
}

bool secPalEscribir(SecPal* sec,const Palabra* pal){
  const char* i = pal->vPal;

  while(*i){
    *sec->cursor = *i;
    i++;
    sec->cursor++;
  }

  *sec->cursor = ' ';

  return true;
}

bool secPalEscribirCaracter(SecPal* sec, char c){
  *sec->cursor = c;
  sec->cursor++;

  return true;
}

bool secPalFin(const SecPal* sec){
  return sec->finSec;
}

void secPalCerrar(SecPal* sec){
  *sec->cursor = '\0';
}
