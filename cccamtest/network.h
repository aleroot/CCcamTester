//
//  network.h
//  cccamtest
//

#ifndef network_h
#define network_h

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

int set_nonblock(int32_t fd, bool nonblock);
int32_t network_tcp_connection_open(char* ip, int port);
int32_t network_tcp_connection_close(int32_t fd);
char* lookup_host (const char *host);

#endif /* network_h */
