#!/bin/bash
# Ejecuta el tp con 20 productores y 1500 registros de prueba
./consumidor 20 1500 &
CONS_PID=$!
echo "Consumidor pid = $CONS_PID"

# Mostrar procesos
ps -ef | grep consumidor | grep -v grep

# Mostrar memoria compartida (system V)
echo "ipcs -m:"
ipcs -m

# Mostrar /dev/shm (si aplica)
echo "/dev/shm:"
ls -la /dev/shm

# Esperar a que termine
wait $CONS_PID
EXIT=$?
echo "tp terminó con código $EXIT"
echo "Últimas 10 líneas de registros.csv:"
tail -n 10 registros.csv
