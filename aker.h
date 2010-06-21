#ifndef AKER_H
#define AKER_H

typedef struct aker_server
{
	int key_count;
    TCMAP *book;
} Server;

typedef struct aker_value
{
    time_t expire;
    size_t size;
    void *data;
} Item;

typedef int aker_callback(struct aker_server *, void *, TCLIST *, void *);

typedef struct aker_command
{
	const char *name;
	aker_callback *callback;
} Command;

#define NOTDEADYET printf("-- STILL OK IN %s AT %d --\n", __FILE__, __LINE__);

#define ERROR(...) fprintf(stderr, __VA_ARGS__);

#define COMMAND(N) static int \
	aker_command_##N(struct aker_server *server, void *socket, TCLIST *args, void *data)

struct aker_server *
Server_create(void);
		
int 
Server_recognize(struct aker_server *, void *, zmq_msg_t *);

int
Server_serve(struct aker_server *);

#endif
