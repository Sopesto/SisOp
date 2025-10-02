#ifndef PALABRA_H_INCLUDED
#define PALABRA_H_INCLUDED

  #include <stdbool.h>
  #define TAM_PALABRA 51

  typedef struct{
    char vPal[TAM_PALABRA];
  }Palabra;

  typedef struct{
    char* cursor;
    bool  finSec;
  } SecPal;

  void palabraATitulo(Palabra*);
  void secPalCrear(SecPal*,char*);
  bool secPalLeer(SecPal*,Palabra*);
  bool secPalEscribir(SecPal*,const Palabra*);
  bool secPalEscribirCaracter(SecPal*,char);
  bool secPalFin(const SecPal*);
  void secPalCerrar(SecPal*);

#endif // PALABRA_H_INCLUDED
