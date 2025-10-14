CC = gcc
CFLAGS = -w -O2
LDFLAGS = -pthread

all: carpetas bin pruebas

carpetas: cobjetos cpruebas cejecutables

pruebas: prueba_consumidor prueba_servidor prueba_cliente
	cp netconfig.config ./Pruebas
	
bin: consumidor servidor cliente
	cp netconfig.config ./Ejecutables

cobjetos:
	@mkdir -p Objetos

cpruebas:
	@mkdir -p Pruebas
	
cejecutables:
	@mkdir -p Ejecutables

consumidor: consumidor.o registro.o
	$(CC) $(CFLAGS) ./Objetos/consumidor.o ./Objetos/registro.o -o ./Ejecutables/consumidor  $(LDFLAGS)

consumidor.o: consumidor.c registro.h
	$(CC) $(CFLAGS) -c consumidor.c -o ./Objetos/consumidor.o

servidor: servidor.o registro.o
	$(CC) $(CFLAGS) ./Objetos/servidor.o ./Objetos/registro.o -o ./Ejecutables/servidor  $(LDFLAGS)

servidor.o: servidor.c registro.h
	$(CC) $(CFLAGS) -c servidor.c -o ./Objetos/servidor.o

cliente: cliente.o registro.o
	$(CC) $(CFLAGS) ./Objetos/cliente.o ./Objetos/registro.o -o ./Ejecutables/cliente 

cliente.o: cliente.c registro.h
	$(CC) $(CFLAGS) -c cliente.c -o ./Objetos/cliente.o

registro.o: registro.c registro.h
	$(CC) $(CFLAGS) -c registro.c -o ./Objetos/registro.o
	
prueba_consumidor: consumidor.o registro.o
	$(CC) $(CFLAGS) ./Objetos/consumidor.o ./Objetos/registro.o -o ./Pruebas/consumidor $(LDFLAGS)

prueba_servidor: servidor.o registro.o
	$(CC) $(CFLAGS) ./Objetos/servidor.o ./Objetos/registro.o -o ./Pruebas/servidor $(LDFLAGS)

prueba_cliente: cliente.o registro.o
	$(CC) $(CFLAGS) ./Objetos/cliente.o ./Objetos/registro.o -o ./Pruebas/cliente 

clean:
	rm -f Objetos/* Pruebas/* Ejecutables/*
.PHONY: all clean

