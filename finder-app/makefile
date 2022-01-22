CC=gcc
CFLAGS=-I -g -Wall -Werror

all: writer

writer: writer.o
	$(CROSS_COMPILE)$(CC) -o writer writer.o

app=writer
	
.PHONY: clean
clean:
	rm -f $(app) *.o
