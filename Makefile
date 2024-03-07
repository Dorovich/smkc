all: smkc

smkc: smkc.c
	gcc -o smkc smkc.c -lmpdclient
