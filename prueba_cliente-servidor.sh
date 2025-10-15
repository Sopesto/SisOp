#!/bin/bash
# Declara variables con el nombre de los archivos a escribir
ARCH_SERV="salida_servidor.txt"
ARCH_USER="salida_cliente.txt"
ARCH_INSC="instrucciones.txt"
ARCH_INSE="instrucciones_sincommit.txt"
ARCH_RESL="resultados.txt"

# Crea el archivo de resultados
touch $ARCH_RESL

# Crea el archivo de configuración de red
echo "192.168.1.42:50001" > netconfig.config

# Ejecuta el servidor con 2 hilos de escucha y 2 espacios de espera
echo "" > $ARCH_SERV
./servidor 2 2 &> $ARCH_SERV &
SERV_PID=$!
echo "Servidor pid = $SERV_PID"

# Archivos abiertos
echo " " > $ARCH_RESL
echo "Archivos abiertos por el servidor" >> $ARCH_RESL
lsof -c servidor >> $ARCH_RESL

# Información del servidor
echo " " >> $ARCH_RESL
echo "Información del servidor" >> $ARCH_RESL
netstat -tnlp | grep ./servidor >> $ARCH_RESL

# Crea 5 procesos clientes de los cuales 4 deben quedar vivos
sleep 5
touch cliente1.txt
./cliente 192.168.1.42 50001 < $ARCH_INSE &> cliente1.txt &
CLI1_PID=$!
echo "Cliente 1 pid = $CLI1_PID"

touch cliente2.txt
./cliente 192.168.1.42 50001 < $ARCH_INSC &> cliente2.txt &
CLI2_PID=$!
echo "Cliente 2 pid = $CLI2_PID"

touch cliente3.txt
./cliente 192.168.1.42 50001 < $ARCH_INSC &> cliente3.txt &
CLI3_PID=$!
echo "Cliente 3 pid = $CLI3_PID"

touch cliente4.txt
./cliente 192.168.1.42 50001 < $ARCH_INSE &> cliente4.txt &
CLI4_PID=$!
echo "Cliente 4 pid = $CLI4_PID"

touch cliente5.txt
./cliente 192.168.1.42 50001 &> cliente5.txt &
CLI5_PID=$!
echo "Cliente 5 pid = $CLI5_PID"

echo " " >> $ARCH_RESL
echo "Procesos clientes" >> $ARCH_RESL
ps -ef | grep cliente | grep -v grep >> $ARCH_RESL

# Conexiones abiertas
echo " " >> $ARCH_RESL
echo "Conexiones abiertas" >> $ARCH_RESL
netstat | grep 50001 >> $ARCH_RESL

# Archivos lockeados
echo " " >> $ARCH_RESL
echo "Archivos lockeados" >> $ARCH_RESL
lslocks | grep servidor >> $ARCH_RESL

# Archivo temporal
echo " " >> $ARCH_RESL
echo "Archivo temporal:" >> $ARCH_RESL
ls | grep temp >> $ARCH_RESL

# Espera a que los procesos terminen  Mata los procesos para que no se guarden sus cambios
sleep 2
wait $CLI5_PID
EXIT=$?
echo "Proceso $CLI5_PID terminó con código $EXIT"
sleep 2
kill $CLI1_PID
EXIT=$?
echo "Proceso $CLI1_PID terminó con código $EXIT"
sleep 2
wait $CLI2_PID
EXIT=$?
echo "Proceso $CLI2_PID terminó con código $EXIT"
sleep 2
wait $CLI3_PID
EXIT=$?
echo "Proceso $CLI3_PID terminó con código $EXIT"
sleep 2
kill $CLI4_PID
EXIT=$?
echo "Proceso $CLI4_PID terminó con código $EXIT"

# Conexiones abiertas
echo " " >> $ARCH_RESL
echo "Conexiones abiertas" >> $ARCH_RESL
netstat | grep 50001 >> $ARCH_RESL

# Archivos lockeados - NO deben haber archivos lockeados
echo " " >> $ARCH_RESL
echo "Archivos lockeados (NO deben haber archivos lockeados)" >> $ARCH_RESL
lslocks | grep servidor >> $ARCH_RESL

# No deben haber clientes abiertos
echo " " >> $ARCH_RESL
echo "Procesos clientes (NO deben haber clientes abiertos)" >> $ARCH_RESL
ps -ef | grep cliente | grep -v grep >> $ARCH_RESL

wait $SERV_PID

echo "Servidor finalizado" >> $ARCH_RESL

