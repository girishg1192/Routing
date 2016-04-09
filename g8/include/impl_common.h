#ifndef _____SOCK_IMPL_____
#define _____SOCK_IMPL_____
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netdb.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef int SOCKET;
#define LOG printf

SOCKET create_socket(struct addrinfo **servinfo_, char* host, char *port);
void check_error(int err, char *func);
#define MAX_NUMBER 20

#endif

