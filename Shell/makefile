.PHONY: all clean

all: myshell

myshell: main.o linkedlist.o
	gcc -o myshell main.o linkedlist.o


myshell.o: main.c linkedlist.h
	gcc -c main.c


linkedlist.o: linkedlist.c
	gcc -c linkedlist.c

clean:
	rm -rf *.o myshell