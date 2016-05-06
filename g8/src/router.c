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
  data_packet buffer;
  int ret = recv(sock, &buffer, sizeof(data_packet), 0);
  char IP[INET_ADDRSTRLEN];
  ip_readable(buffer.dest_ip, IP);
  if(ret==0)
  {
    clear_fd(sock);
    close(sock);
    return;
  }
  LOG("ROUTER: Received %d:%s\n", ret, IP);
  if((--buffer.ttl)==0)
    return;
 // LOG("%s\n", buffer.payload);
  //TODO save sequence numbers to a stupid list or something
  file_stats *incoming_packet;
  if((incoming_packet = find_file_transfer_id(buffer.transfer_id)) == NULL)
  {
    incoming_packet = malloc(sizeof(file_stats));
    memset(incoming_packet, 0, sizeof(file_stats));
    incoming_packet->transfer_id = buffer.transfer_id;
    incoming_packet->ttl = buffer.ttl;
    incoming_packet->current= (incoming_packet->seq_no);
    memcpy(incoming_packet->current, &(buffer.seq_no), sizeof(uint16_t));
    incoming_packet->current +=sizeof(uint16_t);
    incoming_packet->count++;
    insert_file(incoming_packet);
  }
  else
  {
    memcpy(incoming_packet->current, &(buffer.seq_no), sizeof(uint16_t));
    incoming_packet->current +=sizeof(uint16_t);
    incoming_packet->count++;
    LOG("%s\n", incoming_packet->seq_no);
  }
  if(local_ip != buffer.dest_ip)
  {
    int nexthop_index = find_nexthop_by_ip(buffer.dest_ip);
    LOG("Nexthop %d port %d\n", router_list[nexthop_index].id, 
        router_list[nexthop_index].port_data);
    SOCKET nexthop_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in in;
    bzero(&in, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = router_list[nexthop_index].ip;
    in.sin_port = htons(router_list[nexthop_index].port_data);
    int err= connect(nexthop_sock, (struct sockaddr *)&in, sizeof(in));
    check_error(err, "Sendfile connect");
    send(nexthop_sock, &buffer, ret, 0);
    memcpy(&not_last, &last_packet, sizeof(data_packet));
    memcpy(&last_packet, &buffer, sizeof(data_packet));
  }
  else
  {
    if(incoming_packet->fp==NULL)
    {
      char file_name[50];
      sprintf(file_name, "file-%d", buffer.transfer_id);
      incoming_packet->fp = fopen(file_name, "w+");
      perror("File open failed");
    }
    fwrite(buffer.payload, DATA_SIZE, 1, incoming_packet->fp);
    if(buffer.fin)
    {
      fclose(incoming_packet->fp);
      incoming_packet->fp = NULL;
    }
    //TODO packets at dest save to a file
  }
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

  timer_elem *temp;
#if 0
  if((temp = find_timeout_by_ip(addr.sin_addr.s_addr))==NULL)
  {
    struct timer_elem *in = malloc(sizeof(struct timer_elem));
    memset(in, 0, sizeof(struct timer_elem));
    int index = find_router_by_ip(addr.sin_addr.s_addr);
    in->ip = router_list[index].ip;
    in->port = router_list[index].port_routing;
    gettimeofday(&(in->timeout), NULL);
    in->timeout.tv_sec += timeout;
    LOG("Next timeout router %d\n", in->timeout.tv_sec);
//    list_push(in);
    list_insert_ordered(in);
  }
  else
  {
    list_remove(temp);
    gettimeofday(&(temp->timeout), NULL);
    temp->timeout.tv_sec += timeout;
    LOG("Next timeout router %d\n", temp->timeout.tv_sec);
    temp->failures = 0;
    //list_push(temp);
    list_insert_ordered(temp);
  }
#endif

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
    if(router_list[index].nexthop_id = source.id)
    {
      router_list[index].cost = source.cost + router_cost;
    }
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
  LOG("\n\nRouting updates\n\n");
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
    char IP[INET_ADDRSTRLEN];
    ip_readable(router_list[i].ip, IP);
    LOG("Router: %d: ports %d %d\n cost:%d Nexthop:%d IP:%s\n",
        router_list[i].id, router_list[i].port_routing, router_list[i].port_data, 
        router_list[i].cost, router_list[i].nexthop_id, IP);
  }
  for(int i=0; i<router_count; i++)
  {
    if(!router_list[i].neighbour)
      continue;
    LOG("Sending data to %d %d\n", router_list[i].id, router_list[i].port_routing);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in in;
    bzero(&in, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = router_list[i].ip;
    in.sin_port = htons(router_list[i].port_routing);
    sendto(sock, final, size, 0,
        (struct sockaddr *)&in, sizeof(in));
  }
  free(final);
}
