cc -O2 -lconfig -lcurl -pthread jsmn.c exec_build.c api.c main.c -o builder
./builder
