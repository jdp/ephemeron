struct aker_server
{
	int key_count;
    TCMAP *book;
};

struct aker_value
{
    time_t expire;
    unsigned long size;
    void *data;
};

struct aker_response
{
    char code;
    const char *text;
};

typedef int aker_callback(struct aker_server *, TCLIST *, void *);

struct aker_command
{
	const char *name;
	aker_callback *callback;
};
