protocol = "tcp"
host = "127.0.0.1"
port = 5555

ttl_extend = 3600

function on_get(key, value)
	print(string.format("GET {\"%s\":\"%s\"}", key, value))
end

function on_set(key, value)
	print(string.format("SET {\"%s\":\"%s\"}", key, value))
end

function on_expire(key, value)
	print(string.format("EXPIRED {\"%s\":\"%s\"}", key, value))
end
