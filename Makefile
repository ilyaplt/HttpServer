all:
	gcc main.c ring.c utils.c conn.c http_serializer.c -o main -luring -O3
