#ifndef SERVER_H
#define SERVER_H

typedef struct fmdb_server
{
	const char     *config_file;
	const char     *host;
	unsigned short  port;
	int             key_count;
	int             ttl_extension;
	lua_State      *script_env;
    TCMAP          *book;
} Server;

typedef struct fmdb_item
{
    time_t  expire;
    size_t  size;
	char   *key;
    void   *data;
} Item;

typedef int fmdb_command_fn(struct fmdb_server *, void *, TCLIST *, void *);

typedef struct fmdb_command
{
	const char      *name;
	fmdb_command_fn *fn;
} Command;

#define COMMAND(N) static int \
	fmdb_command_##N(struct fmdb_server *server, void *socket, TCLIST *args, void *data)
	
#define L server->script_env

/* Function prototypes */

struct fmdb_server *
Server_create(void);

int
Server_configure(struct fmdb_server *server);
		
int 
Server_react(struct fmdb_server *, void *, zmq_msg_t *);

int
Server_reply_value(struct fmdb_server *, void *, struct fmdb_item *);

int
Server_call_callback(struct fmdb_server *, void *, const char *, struct fmdb_item *);

int
Server_serve(struct fmdb_server *);

#endif
