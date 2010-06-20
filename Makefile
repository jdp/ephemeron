SRC = server.c
OBJ = ${SRC:.c=.o}
CC = gcc
CFLAGS = `pkg-config --cflags libzmq`
LIB = `pkg-config --libs libzmq` -ltokyocabinet -lz -lbz2 -lpthread -lm -lc
OUT = akerd

$(OUT): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIB)
	
.c.o:
	$(CC) $(CFLAGS) -c $<
	
clean:
	rm $(OBJ)
	rm $(OUT)
	
PHONY: clean
