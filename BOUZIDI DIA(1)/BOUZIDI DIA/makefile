all: serveur client ctrl clean

serveur: serveur.o
	gcc -o serveur serveur.o -lm -lpthread
	
seveur.o: serveur.c
	gcc -c serveur.c

client: client.o
	gcc -o client client.o
	
client.o: client.c
	gcc -c client.c

ctrl: ctrl.o
	gcc -o ctrl control_srv.o
	
ctrl.o: control_srv.c
	gcc -c control_srv.c

clean:
	rm *.o
