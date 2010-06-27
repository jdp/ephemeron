protocol = "tcp"
host = "localhost"
port = 5555

ttl_extend = 3600

function on_get(key, value)
  print(string.format("GET {\"%s\":\"%s\"}", key, value))
end

function on_set(key, value)
  print(string.format("SET {\"%s\":\"%s\"}", key, value))
end
