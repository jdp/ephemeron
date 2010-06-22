protocol = "tcp"
host = "localhost"
port = 5555

ttl_extend = 3600

function on_get(key, value)
  print("GET OPERATION")
  print("key:", key)
  print("value:", value)
end

function on_set(key, value)
  print("SET OPERATION")
  print("key:", key)
  print("value:", value)
end
