# Network Connection Stuff
Operations:
- Send    (Get connection from ID)
- Receive (Check IP against existing connections, add connection if not)
- Free    (Free an existing id and remove the associated connection)

It should be fast to:
- Get connection from id
- Iterate through existing connections

It can be slower to:
- Free an existing connection slot
- Add a connection

So:
- The connections should be packed.
- And the connection lookup table should be sparse.

Connection:
- int id
- STRUCT ip

Server:
- STRUCT connections[CON_SIZE]
- int connections_len
- int lookup[LUT_SIZE]

free (id) {
	if(lookup[id] != connections_len - 1) {
		replacement = connections[connections_len - 1]
		connections[lookup[id]] = replacement
		lookup[replacement.id] = lookup[id]
	}
	connections_len--;
	lookup[id] = -1
}

send (id) {
	connection = &connections[lookup[id]]
	ip_send(connection)
}

receive () -> packet {
	ip = ip_receive()
	matched = false

	for(i : 0..connections_len) {
		// matched connection
		if(connections[i] == ip) {
			packet.id = connections[i].id
			matched = true
		}
	}

	if(!matched) {
		// add connection
		id = -1
		for(i : 0..LUT_SIZE) {
			if(lookup[i] == -1) {
				lookup[i] = connections_len
				connections[connections_len] = {.id = i; .ip = ip}
				connections_len++

				packet.id = i
			}
		}
		if(id == -1) {
			PANIC("too many connections")
		}
	}
}
