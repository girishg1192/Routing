#ifndef _____SOCK_IMPL_____
#define _____SOCK_IMPL_____
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netdb.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

typedef int SOCKET;
#define LOG printf

//router list as array
#define ARRAY_ROUTER

SOCKET create_socket_on_port(int port, int stream);
void check_error(int err, char *func);
void ip_readable(uint32_t ip, char *IP);
int find_router_by_port_ip(uint16_t port, uint32_t ip);
int find_index_by_id(uint16_t id);
int find_nexthop_by_ip(uint32_t ip);
#define MAX_NUMBER 20

int router_data, router_control;
SOCKET router_data_sock, router_control_sock;
uint32_t local_ip;
#define ROUTER_INFO_HEADER 12
#define UINT16_T_MAX 65535

//Routin info, populated by controller
//Used by routing.c
struct router_info
{
  uint16_t id;
  uint16_t port_routing;
  uint16_t port_data;
  uint16_t cost;
  uint32_t ip;
  uint16_t nexthop_id;
  uint16_t nexthop_index;
  bool neighbour;
};
typedef struct router_info router_info;
struct timer_elem
{
  uint32_t timeout;
  uint32_t ip;
  uint16_t port;
  bool update;
  uint8_t failures;   //if failures reach 3 drop neighbour
  struct timer_elem *next;
  struct timer_elem *prev;
};
typedef struct timer_elem timer_elem;


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

uint16_t router_count;
uint16_t timeout;
void print_buffer(char *data, int ret);

#define DATA_CONTROLLER_HEADER_SIZE 8
struct data_packet
{
  uint32_t dest_ip;
  uint8_t transfer_id;
  uint8_t ttl;
  uint16_t seq_no;
  uint32_t fin; //change to bitfield
  char payload[1024];
};
typedef struct data_packet data_packet;

data_packet not_last, last;
#define SENDFILE_STATS_HEADER 4
struct file_stats
{
  uint8_t transfer_id;
  uint8_t ttl;
  uint16_t padding;
  char seq_no[20];
  char *current;
  FILE *fp;
  int count;
  struct file_stats *next;
};
typedef struct file_stats file_stats;

#endif

