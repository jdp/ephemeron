# Aker: a cache-and-release database

## How It Works

Each key in the database is assigned an expiry time in the form of a Unix timestamp.
Every time a key is accessed on a read or write operation, its time-to-live is extended.
When a key finally expires, one of two things can happen.
The first option the key name and associated data is broadcast to listening 0MQ-enabled subscribe clients, for them to do what they want with.
Alternatively, a Lua callback function is called with the key name and data as arguments.

## Client Protocol Specification

Aker communicates exclusively over 0MQ.

### GET key-name

Requests the value of a key.
|`key-name`|the name of the key|
Aker will respond with a bulk reply containing the value of the key.

### SET key-name data-size <new-line> data

Sets the value of a key.
|`key-name`|the name of the key|
|`data-size`|the number of bytes that the key's value occupies|
|`data`|the value assigned to the key|
Aker will respond with a success or error message.

