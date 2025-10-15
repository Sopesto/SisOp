CC = gcc
CFLAGS = -w -O2
LDFLAGS = -pthread
ARCH_INSC = "./Pruebas/instrucciones.txt"
ARCH_INSE = "./Pruebas/instrucciones_sincommit.txt"

all: carpetas bin pruebas

carpetas: cobjetos cpruebas cejecutables

pruebas: prueba_consumidor prueba_servidor prueba_cliente archivos_prueba
	echo "192.168.1.42:50001" > ./Pruebas/netconfig.config
	
bin: consumidor servidor cliente
	echo "192.168.1.42:50001" > ./Ejecutables/netconfig.config

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

archivos_prueba:
	touch $(ARCH_INSC)
	touch $(ARCH_INSE)
	echo "AYUDAR" >> $(ARCH_INSC)
	echo "BEGIN TRANSACTION" >> $(ARCH_INSC)
	echo "ORDENAR ID ASC" >> $(ARCH_INSC)
	echo "ELIMINAR ID 2" >> $(ARCH_INSC)
	echo "COMMIT TRANSACTION" >> $(ARCH_INSC)
	echo "SALIR" >> $(ARCH_INSC)
	echo "" >> $(ARCH_INSC)
	echo "AYUDA" >> $(ARCH_INSE)
	echo "BEGIN TRANSACTION" >> $(ARCH_INSE)
	echo "ORDENAR ID DESC" >> $(ARCH_INSE)
	echo "ELIMINAR ID 30" >> $(ARCH_INSE)
	cp ./prueba_consumidor.sh ./Pruebas/
	cp ./prueba_cliente-servidor.sh ./Pruebas/
	cp ./validar_ids.awk ./Pruebas/

clean:
	rm -f Objetos/* Pruebas/* Ejecutables/*
.PHONY: all clean

