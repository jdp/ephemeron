SRC = server.c
OBJ = ${SRC:.c=.o}
CC = gcc
CFLAGS = `pkg-config --cflags libzmq`
LIB = `pkg-config --libs libzmq`
OUT = car

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIB)
	
.c.o:
	$(CC) $(CFLAGS) -c $<
	
clean:
	rm $(OBJ)
	rm $(OUT)
	
PHONY: clean
