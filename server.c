#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zmq.h>

int
serve()
{
	int rc;
	void *ctx, *s;
	zmq_msg_t query, resultset;
	const char *query_string, *resultset_string = "OK";
 
	/* Initialise 0MQ context, requesting a single I/O thread */
	ctx = zmq_init(1);
	assert (ctx);
	/* Create a ZMQ_REP socket to receive requests and send replies */
	s = zmq_socket(ctx, ZMQ_REP);
	assert (s);
	/* Bind to the TCP transport and port 5555 on the 'lo' interface */
	rc = zmq_bind(s, "tcp://lo:5555");
	assert (rc == 0);

	while (1) {
		/* Allocate an empty message to receive a query into */
		rc = zmq_msg_init(&query);
		assert (rc == 0);
 
		/* Receive a message, blocks until one is available */
		rc = zmq_recv(s, &query, 0);
		assert (rc == 0);
 
		/* Process the query */
		query_string = (const char *)zmq_msg_data(&query);
		printf("Received query: '%s'\n", query_string);
		zmq_msg_close(&query);
 
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
	serve();	
	return 0;
}

