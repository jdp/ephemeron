#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <tcutil.h>
#include <zmq.h>
#include "aker.h"

#define AKER_COMMAND(N) static int \
                        aker_command_##N(struct aker_server *server, TCLIST *args, void *data)

AKER_COMMAND(get)
{
    int *size = NULL;
    struct aker_value *value = NULL;
    char *key = NULL;
    if (tclistnum(args) != 2) {
        printf("aker::get expects one argument\n");
        return 0;
    }
    key = (char *)tclistval2(args, 1);
    value = (struct aker_value *)tcmapget(server->book, key, strlen(key), size);
    if (size == NULL) {
        printf("couldn't load %s key\n", (char *)key);
        return 0;
    }
    printf("got data: %s\n", (char *)(value->data));
	return 1;
}

AKER_COMMAND(set)
{
	printf("executing set w/ data %s!\n", (char *)data);
	return 1;
}

static struct aker_command *command_table;
static struct aker_command dirty_command_table[] = {
	{"get", aker_command_get},
	{"set", aker_command_set}
};

static int
sort_command_cb(const void *c1, const void *c2) {
    return strcasecmp(
        ((struct aker_command *)c1)->name,
        ((struct aker_command *)c2)->name);
}


static void
sort_command_table() {
    command_table = (struct aker_command *)malloc(sizeof(dirty_command_table));
    memcpy(command_table, dirty_command_table, sizeof(dirty_command_table));
    qsort(command_table,
        sizeof(dirty_command_table) / sizeof(struct aker_command),
        sizeof(struct aker_command), sort_command_cb);
}

static struct
aker_command *lookup_command(const char *name) {
    struct aker_command tmp = { name, NULL };
    return bsearch(
        &tmp,
        command_table,
        sizeof(dirty_command_table) / sizeof(struct aker_command),
        sizeof(struct aker_command),
        sort_command_cb);
}

int 
recognize(struct aker_server *server, zmq_msg_t *msg)
{
	size_t cmdlen, msglen, datalen;
	char *eol, *message, *command;
	TCLIST *args;
	struct aker_command *cmd;
    void *data = NULL;
	
	message = zmq_msg_data(msg);
    msglen = zmq_msg_size(msg);
    zmq_msg_close(msg);
    
	eol = strchr(message, '\n');
	
	if (eol) {
		cmdlen = 1 + (eol - message);
		command = (char *)malloc((cmdlen + 1) * sizeof(char));
		memset(command, 0, cmdlen + 1);
		memcpy(command, message, cmdlen);
		args = tcstrsplit(tcstrsqzspc(command), " \t");
		cmd = lookup_command(tclistval2(args, 0));
		if (cmd == NULL) {
            printf("protocol error:\n\t`%s'\n", command);
			return 0;
		}
		else {
            printf("got command: %s\n", cmd->name);
		}
        printf("%s, command size: %lu, msg size: %lu\n", cmd->name, cmdlen, msglen);
        if (msglen > cmdlen) {
            /* data size will always be last arg */
            datalen = tcatoi((char *)tclistval2(args, tclistnum(args) - 1));
            data = malloc(sizeof(char) * (datalen + 1));
            memset(data + datalen, 0, 1);
            memcpy(data, message + cmdlen, datalen);
        }
        cmd->callback(server, args, data);
		return 1;
	}

	return 0;
}

struct aker_server *
make_server(void)
{
    struct aker_server *server = NULL;
    server = (struct aker_server *)malloc(sizeof(struct aker_server));
    if (server == NULL) {
        return NULL;
    }
    server->key_count = 0;
    server->book = tcmapnew();
    return server;
}

int
serve()
{
	int rc;
	void *ctx, *s;
	zmq_msg_t query, resultset;
	const char *cmd_name, *query_string, *resultset_string = "OK";
    struct aker_server *server = make_server();
 
	/* Initialise 0MQ context, requesting a single I/O thread */
	ctx = zmq_init(1);
	assert (ctx);
	/* Create a ZMQ_REP socket to receive requests and send replies */
	s = zmq_socket(ctx, ZMQ_REP);
	assert (s);
	/* Bind to the TCP transport and port 5555 on the 'lo' interface */
	rc = zmq_bind(s, "tcp://127.0.0.1:5555");
	assert (rc == 0);

	while (1) {
		/* Allocate an empty message to receive a query into */
		rc = zmq_msg_init(&query);
		assert (rc == 0);
 
		/* Receive a message, blocks until one is available */
		rc = zmq_recv(s, &query, 0);
		assert (rc == 0);
 
		/* Process the query */
		if (!recognize(server, &query)) {
			printf("unrecognized command\n");
		}
 
		/* Allocate a response message and fill in an example response */
		rc = zmq_msg_init_size(&resultset, strlen (resultset_string) + 1);
		assert (rc == 0);
		memcpy(zmq_msg_data(&resultset), resultset_string, strlen(resultset_string) + 1);
 
		/* Send back our canned response */
		rc = zmq_send(s, &resultset, 0);
		assert (rc == 0);
		zmq_msg_close(&resultset);
	}
}
 
int
main(int argc, char **argv) 
{
	sort_command_table();
	serve();	
	return 0;
}

