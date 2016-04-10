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

//router list as array
#define ARRAY_ROUTER

SOCKET create_socket(struct addrinfo **servinfo_, char* host, char *port);
void check_error(int err, char *func);
#define MAX_NUMBER 20

//Routin info, populated by controller
//Used by routing.c
struct router_info
{
  uint16_t id;
  uint16_t port_routing;
  uint16_t port_data;
  uint16_t cost;
  uint32_t ip;
};
typedef struct router_info router_info;


#ifdef ARRAY_ROUTER
router_info *router_list;
#else
struct list_elem
{
  router_info router_inf;
  struct list *next;
  struct list *prev;
};

typedef struct list_elem list_elem;
list_elem *head;
list_elem *tail;
#endif

uint8_t router_count;
uint8_t timeout;

#endif

