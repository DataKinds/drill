all:
	gcc -Wall -g -o drill main.c

nr: all
	./drill
	
dbg: all
	gdb drill