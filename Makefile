SRC = main.c util.c server.c
OBJ = ${SRC:.c=.o}
CC = gcc
CFLAGS = `pkg-config --cflags libzmq tokyocabinet lua5.1`
LIB = `pkg-config --libs libzmq tokyocabinet lua5.1`
OUT = ephemd

$(OUT): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)
	
.c.o:
	$(CC) $(CFLAGS) -c $<
	
clean:
	rm $(OBJ)
	rm $(OUT)
	
PHONY: clean
