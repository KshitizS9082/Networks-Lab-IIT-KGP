all:
	gcc -c rsocket.c -pthread
	ar -rcs librsocket.a rsocket.o
	gcc user1.c -L. -lrsocket -pthread -o u1
	gcc user2.c -L. -lrsocket -pthread -o u2
clean:
	rm u1 u2 rsocket.o librsocket.a