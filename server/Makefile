CC=$(CROSS_COMPILE)gcc
CFLAGS=-Werror -Wall -g

TARGET=aesdsocket

all: aesdsocket
default: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o	

.PHONY : clean
clean :
	rm -f $(TARGET) *.o
