.PHONY: clean sendprog
all: sendprog

sendprog: sendprog.c
	gcc -g -Wall sendprog.c output.c input.c -lpthread -o sendprog

clean:
	rm -f sendprog
