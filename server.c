#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <tcutil.h>
#include <zmq.h>
#include "aker.h"

static int
send(void *socket, char *str)
{
	int rc;
	zmq_msg_t msg;
	
	rc = zmq_msg_init_size(&msg, strlen(str) + 1);
	assert(rc == 0);
	memcpy(zmq_msg_data(&msg), str, strlen(str) + 1);
	rc = zmq_send(socket, &msg, 0);
	assert(rc == 0);
	zmq_msg_close(&msg);
	return 1;
}

static int
rawsend(void *socket, void *data, size_t size)
{
	int rc;
	zmq_msg_t msg;

	rc = zmq_msg_init_size(&msg, size);
	assert(rc == 0);
	memcpy(zmq_msg_data(&msg), data, size);
	rc = zmq_send(socket, &msg, 0);
	assert(rc == 0);
	zmq_msg_close(&msg);
	return 1;
}

COMMAND(get)
{
	int size = 0;
	int digits = 0;
	struct aker_value *value = NULL;
	char *key = NULL, *digits_str = NULL;
	void *response = NULL;
	
	if (tclistnum(args) != 2) {
		send(socket, "-TOO_FEW_ARGS");
		return 0;
	}
	key = (char *)tclistval2(args, 1);
	value = (struct aker_value *)tcmapget(server->book, key, strlen(key), &size);
	if (size == 0) {
		send(socket, "-NO_SUCH_KEY");
		return 0;
	}
	digits = (int)(log(value->size)/log(10)) + 1;
	digits_str = (char *)malloc(sizeof(char)*(digits + 1));
	memset(digits_str, 0, digits + 1);
	sprintf(digits_str, "%lu", value->size);
	response = malloc(value->size + digits + 2);
	memset(response, '$', 1);
	memcpy(response + 1, digits_str, digits);
	memset(response + digits + 1, '\n', 1);
	memcpy(response + digits + 2, value->data, value->size);
	rawsend(socket, response, value->size + digits + 2);
	free(digits_str);
	free(response);
	return 1;
}

COMMAND(set)
{
	char *key = NULL;
	size_t key_size, data_size;
	struct aker_value *value = NULL;
	
	key = (char *)tclistval2(args, 1);
	key_size = (size_t)strlen(key);
	data_size = tcatoi((char *)tclistval2(args, tclistnum(args) - 1));
	
	value = (struct aker_value *)malloc(sizeof(struct aker_value));
	value->expire = time(NULL);
	value->size = data_size;
	value->data = data;
	
	tcmapput(server->book, key, key_size, value, sizeof(*value));
	send(socket, "+OK");
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

static struct aker_command *
lookup_command(const char *name) {
	struct aker_command tmp = { name, NULL };
	return bsearch(
		&tmp,
		command_table,
		sizeof(dirty_command_table) / sizeof(struct aker_command),
		sizeof(struct aker_command),
		sort_command_cb);
}

int 
Server_react(struct aker_server *server, void *socket, zmq_msg_t *msg)
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
			ERROR("protocol error:\n\t`%s'\n", command);
			send(socket, "-INVALID_COMMAND");
			return 0;
		}
		if (msglen > cmdlen) {
			/* data size will always be last arg */
			datalen = tcatoi((char *)tclistval2(args, tclistnum(args) - 1));
			if ((datalen + cmdlen) != msglen) {
				ERROR("error receiving data or invalid data size\n");
				send(socket, "-INCORRECT_DATA_SIZE");
				return 0;
			}
			data = malloc(sizeof(char) * (datalen + 1));
			memset(data + datalen, 0, 1);
			memcpy(data, message + cmdlen, datalen);
		}
		cmd->callback(server, socket, args, data);
		return 1;
	}
	else {
		ERROR("request missing eol");
		send(socket, "-MISSING_EOL");
	}

	return 0;
}

int
Server_serve(struct aker_server *server)
{
	int rc;
	void *ctx, *s;
	zmq_msg_t query, resultset;
	const char *cmd_name, *query_string, *resultset_string = "OK";

	/* Start up the 0MQ server */
	ctx = zmq_init(1);
	assert (ctx);
	s = zmq_socket(ctx, ZMQ_REP);
	assert (s);
	rc = zmq_bind(s, "tcp://127.0.0.1:5555");
	assert (rc == 0);
	
	sort_command_table();

	/* Start the request/reply loop */
	while (1) {
		rc = zmq_msg_init(&query);
		assert (rc == 0);
		rc = zmq_recv(s, &query, 0);
		assert (rc == 0);
		if (!Server_react(server, s, &query)) {
			ERROR("unrecognized command\n");
		}
	}
	
	return 1;
}

struct aker_server *
Server_create(void)
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
main(int argc, char **argv) 
{
	struct aker_server *server;
	
	/* Start up the server */
	server = Server_create();
	if (server == NULL) {
		ERROR("server start failed\n");
		return 1;
	}
	Server_serve(server);
		
	return 0;
}

