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
  char buffer[1000];
  int fromlen = sizeof addr;
  int ret = recvfrom(sock, &buffer, sizeof(buffer), 0,
                  (struct sockaddr *)&addr, &fromlen);
  char IP[INET_ADDRSTRLEN];
  ip_readable(addr.sin_addr.s_addr, IP);
  LOG("ROUTER: Received %d:%s->%x\n", ret, IP, addr.sin_addr.s_addr);
  //TODO handle actual routing and stuff
}
void router_control_receive(SOCKET sock)
{
  struct sockaddr_in addr;
  char buffer[1000];
  int fromlen = sizeof addr;
  int ret = recvfrom(sock, &buffer, sizeof(buffer), 0,
                  (struct sockaddr *)&addr, &fromlen);
  char IP[INET_ADDRSTRLEN];
  ip_readable(addr.sin_addr.s_addr, IP);
  LOG("ROUTER: Received %d:%s->%x\n", ret, IP, addr.sin_addr.s_addr);
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
  for(int i=0; i<router_count; i++)
  {
    uint32_t ip_addr = router_list[i].ip;
    memcpy(data, &ip_addr, sizeof(uint32_t));
    data = data+sizeof(uint32_t);
    uint16_t port = htons(router_list[i].port_routing);
    memcpy(data, &port, sizeof(uint16_t));
    data = data+sizeof(uint32_t);
    uint16_t temp = htons(router_list[i].id);
    memcpy(data, &temp, sizeof(uint16_t));
    data+sizeof(uint16_t);
    temp = htons(router_list[i].cost);
    memcpy(data, &temp, sizeof(uint16_t));
    data+sizeof(uint16_t);
  }
  LOG("Data copied %d %d", size, data-final);
  for(int i=0; i<router_count; i++)
  {
    if(!router_list[i].neighbour)
      continue;
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

