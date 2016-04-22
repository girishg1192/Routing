#include "router.h"
SOCKET start_router_data(int port)
{
  return 0;
}
SOCKET start_router_control(int port)
{
  return 0;
}
void router_data_receive(SOCKET sock)
{
  struct sockaddr_in addr;
  data_packet buffer;
  int fromlen = sizeof addr;
  int ret = recv(sock, &buffer, sizeof(data_packet), 0);
  char IP[INET_ADDRSTRLEN];
  ip_readable(buffer.dest_ip, IP);
  if(ret==0)
  {
    clear_fd(sock);
    close(sock);
  }
  LOG("ROUTER: Received %d:%s\n", ret, IP);
  //TODO handle actual routing and stuff
}

void router_control_receive(SOCKET sock)
{
  struct sockaddr_in addr;
  char *buffer = malloc(1000);
  char *ptr = buffer;
  int fromlen = sizeof addr;
  int ret = recvfrom(sock, buffer, 1000, 0,
                  (struct sockaddr *)&addr, &fromlen);
  char IP[INET_ADDRSTRLEN];
  ip_readable(addr.sin_addr.s_addr, IP);
  LOG("ROUTER Control: Received %d:%s->%x\n", ret, IP, addr.sin_addr.s_addr);

  uint16_t count;
  memcpy(&count, buffer, sizeof(uint16_t));
  count = ntohs(count);
  buffer=buffer+sizeof(uint16_t);

  uint16_t src_port;
  memcpy(&src_port, buffer, sizeof(uint16_t));
  src_port = ntohs(src_port);
  buffer=buffer+sizeof(uint16_t);

  uint32_t src_ip;
  memcpy(&src_ip, buffer, sizeof(uint32_t));
  buffer = buffer+sizeof(uint32_t);
  LOG("Updates : %d from %d %x\n", count, src_port, src_ip);

  int src_index =  find_router_by_port_ip(src_port, src_ip);
  router_info source = router_list[src_index];
  for(int i=0; i<count; i++)
  {
    uint32_t ip_addr;
    memcpy(&ip_addr, buffer, sizeof(uint32_t));
    buffer = buffer + sizeof(uint32_t);

    uint16_t port;
    memcpy(&port, buffer, sizeof(uint16_t));
    buffer = buffer + sizeof(uint32_t);
    
    uint16_t router_id, router_cost;
    memcpy(&router_id, buffer, sizeof(uint16_t));
    router_id = ntohs(router_id);
    buffer = buffer + sizeof(uint16_t);
    memcpy(&router_cost, buffer, sizeof(uint16_t));
    router_cost = ntohs(router_cost);
    buffer = buffer + sizeof(uint16_t);
    int index = find_index_by_id(router_id);
    if(router_list[index].cost>(source.cost + router_cost))
    {
      router_list[index].cost = source.cost + router_cost;
      router_list[index].nexthop_id = source.id;
      router_list[index].nexthop_index = src_index;
      //update route
      LOG("Shorter path to %d through %d\n", router_id, src_index);
    }
  }
  free(ptr);
  //TODO handle actual routing and stuff
}
void router_send_updates()
{
  int size = ROUTER_UPDATE_HEADER + ROUTER_UPDATE_DATA*router_count;
  //Copy data to memory
  char *data = malloc(size);
  memset(data, 0, size);
  char *final = data;
  uint16_t temp;
  temp = htons(router_count);
  memcpy(data, &temp, sizeof(uint16_t));
  data = data+sizeof(uint16_t);
  temp = htons(router_control);
  memcpy(data, &temp, sizeof(uint16_t));
  data = data+sizeof(uint16_t);

  uint32_t ip = local_ip;
  memcpy(data, &ip, sizeof(uint32_t));
  data = data+sizeof(uint32_t);
  LOG("Copied header %d count %d\n", data-final, router_count);
  for(int i=0; i<router_count; i++)
  {
    uint32_t ip_addr = router_list[i].ip;
    memcpy(data, &ip_addr, sizeof(uint32_t));
    data+= sizeof(uint32_t);
    uint16_t port = htons(router_list[i].port_routing);
    memcpy(data, &port, sizeof(uint16_t));
    data+= sizeof(uint32_t);
    uint16_t temp = htons(router_list[i].id);
    memcpy(data, &temp, sizeof(uint16_t));
    data+= sizeof(uint16_t);
    temp = htons(router_list[i].cost);
    memcpy(data, &temp, sizeof(uint16_t));
    data+= sizeof(uint16_t);
  }
  LOG("Data copied %d %d\n", size, data-final);
  for(int i=0; i<router_count; i++)
  {
    if(!router_list[i].neighbour)
      continue;
    LOG("Sending data to %d %d\n", i, router_list[i].port_routing);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in in;
    bzero(&in, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = router_list[i].ip;
    in.sin_port = htons(router_list[i].port_routing);
    sendto(sock, final, size, 0,
        (struct sockaddr *)&in, sizeof(in));
  }
}
