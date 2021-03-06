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

Database will respond with a value or error reply.

### SET key-name data-size <new-line> data

Sets the value of a key.

`key-name` is the name of the key.

`data-size` is the number of bytes that the key's value occupies.

`data` is the value assigned to the key.

*new-line* is a literal newline `'\n'` character. No Exceptions.

Database will respond with a success or error reply.

## Server Protocol Specification

### Success Reply

Successful status messages are prefixed with a `+`.
For example, the SET command will return `+OK` on success.

### Error Reply

When something goes wrong, the server's reply will be prefixed with `-`.
It will also contain a short, unique error name.
For example, when a key does not exist, `-NO_SUCH_KEY` is returned.

### Value Reply

A value reply is made up of two parts: the data size and the data.
Data size is provided to that the data is binary safe, and to ensure nothing went wrong during transmission.
The data size is sent as an integer prefixed by a `$` and followed with a new line `\n`.
The binary data follows the new line.

Say the following command was issued:

    SET garden 7
    octopus

The *garden* key is now set to *octopus*, and the server should have replied with:

    +OK

Now when a GET command for the *garden* key is issued, the response will look like this:

    $7
    octopus

## License

Licensed under the [ISC License](http://www.isc.org/software/license).

Copyright (c) 2010, Justin Poliey <jdp34@njit.edu>

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## Credits

Code by [Justin Poliey](http://justinpoliey.com)

Clear & simple protocol inspired by [Redis](http://redis.io)