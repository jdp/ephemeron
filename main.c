#include <stdio.h>
#include <tcutil.h>
#include <zmq.h>
#include <lua.h>
#include "util.h"
#include "server.h"

int
main(int argc, char **argv) 
{
	Server *server;
	
	/* Start up the server */
	server = Server_create();
	if (server == NULL) {
		ERROR("server create failed\n");
		return 1;
	}
	if (!Server_configure(server)) {
		ERROR("server configure failed\n");
		return 1;
	}
	Server_serve(server);
		
	return 0;
}
