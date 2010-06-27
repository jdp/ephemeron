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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "util.h"
#include "server.h"

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

/*
 * -- COMMAND API BEGINS! --
 */

COMMAND(get)
{
	int size = 0;
	Item *value = NULL;
	char *key = NULL;
	
	/* Ensure enough arguments were passed with GET */
	if (tclistnum(args) != 2) {
		send(socket, "-TOO_FEW_ARGS");
		return 0;
	}
	
	/* Try to fetch the value associated with the key */
	key = (char *)tclistval2(args, 1);
	value = (Item *)tcmapget(server->book, key, strlen(key), &size);
	if (size == 0) {
		send(socket, "-NO_SUCH_KEY");
		return 0;
	}
	
	/* Update the TTL on the item */
	value->expire += server->ttl_extension;
	
	/* Execute callback function if available */
	lua_getglobal(L, "on_get");
	if (lua_isfunction(L, -1)) {
		lua_pushstring(L, key);
		lua_pushlstring(L, (const char *)value->data, value->size);
		if (lua_pcall(L, 2, 1, 0) != 0) {
			ERROR("callback error: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			send(socket, "-CALLBACK_ERROR");
			return 0;
		}
		if (lua_isstring(L, -1)) {
			/* transform key here */
		}
	}
	lua_pop(L, 1);
	
	/* Send back the response */
	return Server_reply_value(server, socket, value);
}

COMMAND(set)
{
	char *key = NULL;
	size_t key_size, data_size;
	Item *value = NULL;
	
	key = (char *)tclistval2(args, 1);
	key_size = (size_t)strlen(key);
	data_size = tcatoi((char *)tclistval2(args, tclistnum(args) - 1));
	
	/* Store the new key */
	value = (Item *)malloc(sizeof(Item));
	if (value == NULL) {
		ERROR("out of memory during set operation");
		send(socket, "-OUT_OF_MEMORY");
		return 0;
	}
	value->expire = time(NULL) + server->ttl_extension;
	value->size = data_size;
	value->data = data;
	
	/* Execute callback function if available */
	lua_getglobal(L, "on_set");
	if (lua_isfunction(L, -1)) {
		lua_pushstring(L, key);
		lua_pushlstring(L, (const char *)value->data, value->size);
		if (lua_pcall(L, 2, 1, 0) != 0) {
			ERROR("callback error:\n\t%s", lua_tostring(L, -1));
			lua_pop(L, 1);
			send(socket, "-CALLBACK_ERROR");
			return 0;
		}
	}
	else {
		lua_pop(L, 1);
	}
	
	/* After callback so it won't be added when callback errors out */
	tcmapput(server->book, key, key_size, value, sizeof(*value));
	
	send(socket, "+OK");
	return 1;
}

/* The command lookup is taken directly from Redis */

static Command *command_table;
static Command dirty_command_table[] = {
	{"get", fmdb_command_get},
	{"set", fmdb_command_set}
};

static int
sort_command_cb(const void *c1, const void *c2) {
	return strcasecmp(((Command *)c1)->name, ((Command *)c2)->name);
}

static void
sort_command_table() {
	command_table = (Command *)malloc(sizeof(dirty_command_table));
	memcpy(command_table, dirty_command_table, sizeof(dirty_command_table));
	qsort(command_table,
		sizeof(dirty_command_table) / sizeof(Command),
		sizeof(Command), sort_command_cb);
}

static Command *
lookup_command(const char *name) {
	Command tmp = { name, NULL };
	return bsearch(
		&tmp,
		command_table,
		sizeof(dirty_command_table) / sizeof(Command),
		sizeof(Command),
		sort_command_cb);
}

int
Server_reply_value(Server *server, void *socket, Item *item)
{
	int digits;
	char *digits_str = NULL;
	void *response = NULL;
	
	digits = (int)(log(item->size)/log(10)) + 1;
	if ((digits_str = (char *)malloc(sizeof(char)*(digits + 1))) == NULL) {
		ERROR("oom: digits string in value response");
		return 0;
	}
	memset(digits_str, 0, digits + 1);
	sprintf(digits_str, "%lu", item->size);
	if ((response = malloc(item->size + digits + 2)) == NULL) {
		ERROR("oom: response in value response");
		return 0;
	}
	memset(response, '$', 1);
	memcpy(response + 1, digits_str, digits);
	memset(response + digits + 1, '\n', 1);
	memcpy(response + digits + 2, item->data, item->size);
	rawsend(socket, response, item->size + digits + 2);
	free(digits_str);
	free(response);
	return 1;
}

int 
Server_react(Server *server, void *socket, zmq_msg_t *msg)
{
	size_t cmdlen, msglen, datalen;
	char *eol, *message, *command;
	TCLIST *args;
	Command *cmd;
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
			ERROR("protocol error:\n\t`%s'", command);
			send(socket, "-INVALID_COMMAND");
			return 0;
		}
		if (msglen > cmdlen) {
			/* data size will always be last arg */
			datalen = tcatoi((char *)tclistval2(args, tclistnum(args) - 1));
			if ((datalen + cmdlen) != msglen) {
				ERROR("error receiving data or invalid data size");
				send(socket, "-INCORRECT_DATA_SIZE");
				return 0;
			}
			data = malloc(sizeof(char) * (datalen + 1));
			if (data == NULL) {
				ERROR("out of memory");
				send(socket, "-OUT_OF_MEMORY");
				return 0;
			}
			memset(data + datalen, 0, 1);
			memcpy(data, message + cmdlen, datalen);
			DEBUG("data received: %s", data);
		}
		cmd->fn(server, socket, args, data);
		return 1;
	}
	else {
		ERROR("request missing eol");
		send(socket, "-MISSING_EOL");
	}

	return 0;
}

int
Server_serve(Server *server)
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
			ERROR("unrecognized command");
		}
	}
	
	return 1;
}

int
Server_configure(Server *server)
{
	if (luaL_dofile(L, server->config_file) != 0) {
		ERROR("could not load config file `%s':\n\t%s",
			server->config_file, lua_tostring(L, -1));
		lua_close(L);
		return 0;
	}
	return 1;
}

Server *
Server_create(void)
{
	Server *server = NULL;
	
	server = (Server *)malloc(sizeof(Server));
	if (server == NULL) {
		ERROR("memory for server not allocated");
		return 0;
	}
	server->config_file = "config.lua";
	server->key_count = 0;
	server->ttl_extension = 3600;
	if ((L = luaL_newstate()) == NULL) {
		ERROR("could not initiate scripting environment");
		return 0;
	}
	luaL_openlibs(L);
	server->book = tcmapnew();
	return server;
}

