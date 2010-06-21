#ifndef SERVER_H
#define SERVER_H

typedef struct aker_server
{
	const char     *config_file;
	const char     *host;
	unsigned short  port;
	int             key_count;
	lua_State      *script_env;
    TCMAP          *book;
} Server;

typedef struct aker_value
{
    time_t  expire;
    size_t  size;
    void   *data;
} Item;

typedef int aker_callback(struct aker_server *, void *, TCLIST *, void *);

typedef struct aker_command
{
	const char    *name;
	aker_callback *callback;
} Command;

#define COMMAND(N) static int \
	aker_command_##N(struct aker_server *server, void *socket, TCLIST *args, void *data)
	
#define L server->script_env

/* Function prototypes */

struct aker_server *
Server_create(void);

int
Server_configure(struct aker_server *server);
		
int 
Server_react(struct aker_server *, void *, zmq_msg_t *);

int
Server_serve(struct aker_server *);

#endif
