all: cnt

cnt: cnt.c
	gcc -o cnt cnt.c -lmpdclient
