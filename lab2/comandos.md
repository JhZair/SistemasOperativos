terminal 1
gcc -Wall -Wextra receptor.c -o receptor
./receptor

terminal 2
gcc -Wall -Wextra emisor.c -o emisor
./emisor

terminal 3
kill -2  <PID>     # → mensaje tipo 1
kill -10 <PID>     # → mensaje tipo 2
kill -12 <PID>     # → mensaje tipo 3

para ejercicio 4
kill -2 <PID>; sleep 1; kill -10 <PID>; sleep 1; kill -12 <PID>; sleep 1
kill -2 <PID>; sleep 1; kill -12 <PID>; sleep 1; kill -10 <PID>

terminal 4
ipcs -q                    # todas las colas del sistema
watch -n 1 ipcs -q         # actualiza cada segundo, ideal para ver la acumulación
ipcs -q -i <msqid>         # detalle de una cola concreta