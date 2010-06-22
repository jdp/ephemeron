# EphemDB: a cache-and-release database

## How It Works

Each key in the database is assigned an expiry time in the form of a Unix timestamp.
Every time a key is accessed on a read or write operation, its time-to-live is extended.
When a key finally expires, one of two things can happen.
The first option the key name and associated data is broadcast to listening 0MQ-enabled subscribe clients, for them to do what they want with.
Alternatively, a Lua callback function is called with the key name and data as arguments.

## Scripting Support

The database is configured with Lua.
There are callbacks available for when keys are accessed.

The `on_get` callback takes two parameters, a key and a value.
If a string value is returned by the callback, that value is returned to the client requesting the key.

## Client Protocol Specification

Database communicates exclusively over 0MQ.

### GET key-name

Requests the value of a key.

`key-name` is the name of the key.

Database will respond with a bulk reply containing the value of the key.

### SET key-name data-size <new-line> data

Sets the value of a key.

`key-name` is the name of the key.

`data-size` is the number of bytes that the key's value occupies.

`data` is the value assigned to the key.

*new-line* is a literal newline `'\n'` character. No Exceptions.

Database will respond with a success or error message.

## Server Protocol Specification

### Success Message

Successful status messages are prefixed with a `+`.
For example, the SET command will return `+OK` on success.

## Credits

Code by [Justin Poliey](http://justinpoliey.com)

Clear & simple protocol inspired by [Redis](http://redis.io)