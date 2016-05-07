#include "impl_common.h"
#if 0
SOCKET create_socket(struct addrinfo **servinfo_, char* host, char *port, int stream)
{
  struct addrinfo *servinfo = *servinfo_;
  struct addrinfo hints;
  
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = stream;

  getaddrinfo(host, port, &hints, &servinfo);
  int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  *servinfo_ = servinfo;
  return sockfd;
}
#endif
void check_error(int err, char* func)
{
  if(err)
  {
    LOG("%s failed = %d\n",func, err);
    perror("Failed!");
  }
}
SOCKET create_socket_on_port(int port, int stream)
{
  LOG("ControlPort: %d\n", port);
  SOCKET server_socket = socket(AF_INET, stream, 0);
  int yes =1;

  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("Socket already in use, setsockopt failed");
    exit(1);
  }
  int final_port = port;
  LOG("ControlPort: listen port %d\n", final_port);

  struct sockaddr_in in;
  bzero(&in, sizeof(in));
  int hport = htons(port);
  in.sin_family = AF_INET;
  in.sin_addr.s_addr = htonl(INADDR_ANY);
  in.sin_port = hport;

  int err = bind(server_socket, (struct sockaddr *) &in, sizeof(in));
  check_error(err, "Bind");

  if(stream!=SOCK_DGRAM)
  {
    err = listen(server_socket, MAX_NUMBER);
    check_error(err, "Listen");
  }
  return server_socket;
}
void ip_readable(uint32_t ip, char *IP)
{
  inet_ntop(AF_INET, &ip, IP, INET_ADDRSTRLEN);
}
void print_buffer(char *data, int ret)
{
  for(int i=0; i<ret; i++)
  {
    if(i%4==0)LOG("\n");
    LOG("%02x ", data[i]);
  }
}
//Find stuff in router_list
int find_router_by_port_ip(uint16_t port, uint32_t ip)
{
  for(int i=0; i<router_count; i++)
  {
    if(router_list[i].ip == ip && router_list[i].port_routing == port)
      return i;
  }
}
int find_router_by_ip(uint32_t ip)
{
  for(int i=0; i<router_count; i++)
  {
    if(router_list[i].ip == ip)
      return i;
  }
}
int find_index_by_id(uint16_t id)
{
  for(int i=0; i<router_count; i++)
  {
    if(router_list[i].id == id)
      return i;
  }
}
int find_nexthop_by_ip(uint32_t ip)
{
  for(int i=0; i<router_count; i++)
  {
    if(router_list[i].ip == ip)
      return router_list[i].nexthop_index;
  }
}
struct timeval check_and_set_timer(struct timeval tv)
{
  if(tv.tv_sec ==0 && tv.tv_usec ==0)
  {
    //Do nothing?
  }
  else
  {
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    timersub(&tv, &curr_time, &tv);
    LOG("Check and set to %d %d\n\n", tv.tv_sec, curr_time.tv_sec);
  }
  return tv;
}
void recalc_routing()
{
  for(int i=0; i<router_count; i++)
  {
    uint16_t min=UINT16_T_MAX;
    timer_elem *min_hop=NULL, *temp;
    if(router_list[i].cost == 0)
      continue;
    TAILQ_FOREACH(temp, &timer_list, next)
    {
      if(temp->update || temp->dv[i].cost==UINT16_T_MAX)
        continue;
      uint16_t node_cost = SUM(temp->cost, temp->dv[i].cost);
      if(node_cost<min)
      {
        min = node_cost;
        min_hop = temp;
      }
    }
    if(min_hop!=NULL)
    {
      router_list[i].cost = min;
      int index_min = find_router_by_ip(min_hop->ip);
      router_list[i].nexthop_id = router_list[index_min].id;
      router_list[i].nexthop_index = index_min;
      LOG("Minimum path to %d through %d : %d\n", router_list[i].id, min_hop->id,
          min);
    }
  }
}
int recv_t(SOCKET sock_index, char *buffer, int nbytes)
{
  int bytes = 0;
  bytes = recv(sock_index, buffer, nbytes, 0);

  if(bytes == 0) return -1;
  while(bytes != nbytes)
    bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);

  return bytes;
}
