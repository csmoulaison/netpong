#ifndef platform_network_h_INCLUDED
#define platform_network_h_INCLUDED

#include "base/base.h"

struct PlatformSocket {

};

// Opens a UDP socket for a server.
PlatformSocket platform_init_server_socket();

// Opens a UDP socket for a client.
PlatformSocket platform_init_client_socket();

#endif // platform_network_h_INCLUDED
