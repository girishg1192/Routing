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
  char IP[INET_ADDRSTRLEN];
  int ret = recv_t(sock, (char *)&buffer, sizeof(data_packet));
  if(ret>0)
  {
    //  if(ret<=0)
    //  {
    //    clear_fd(sock);
    //    close(sock);
    //    return;
    //  }
    //ip_readable(buffer.dest_ip, IP);
    //LOG("ROUTER: Received %d:%s\n", ret, IP);
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
      incoming_packet->data = malloc(FILE_SIZE_MAX);
      memcpy(incoming_packet->current, &(buffer.seq_no), sizeof(uint16_t));
      incoming_packet->current +=sizeof(uint16_t);
      incoming_packet->count++;

      if(buffer.dest_ip!=local_ip)
      {
        int nexthop_index = find_nexthop_by_ip(buffer.dest_ip);
        incoming_packet->sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in in;
        bzero(&in, sizeof(in));
        in.sin_family = AF_INET;
        in.sin_addr.s_addr = router_list[nexthop_index].ip;
        in.sin_port = htons(router_list[nexthop_index].port_data);
        int err= connect(incoming_packet->sock, (struct sockaddr *)&in, sizeof(in));
        check_error(err, "Sendfile connect");
      }
      insert_file(incoming_packet);
    }
    else
    {
      memcpy(incoming_packet->current, &(buffer.seq_no), sizeof(uint16_t));
      incoming_packet->current +=sizeof(uint16_t);
      incoming_packet->count++;
      //LOG("%x\n", ntohs(buffer.seq_no));
    }
    memcpy(&not_last, &last_packet, sizeof(data_packet));
    memcpy(&last_packet, &buffer, sizeof(data_packet));
    if(local_ip != buffer.dest_ip)
    {
      send(incoming_packet->sock, &buffer, ret, 0);
      if(buffer.fin)
        close(incoming_packet->sock);
    }
    else
    {
      memcpy(incoming_packet->data, buffer.payload, DATA_SIZE);
      incoming_packet->data += DATA_SIZE;
      if(buffer.fin)
      {
        char file_name[50];
        sprintf(file_name, "file-%d", buffer.transfer_id);
        FILE *fp = fopen(file_name, "w+");
        incoming_packet->data -=incoming_packet->count*DATA_SIZE;
        fwrite(incoming_packet->data, incoming_packet->count*DATA_SIZE, 1, fp);
        fclose(fp);
        free(incoming_packet->data);
        close(sock);
        clear_fd(sock);
        LOG("Transfer complete\n");
      }
      //TODO packets at dest save to a file
    }
  }
  else
  {
    close(sock);
    clear_fd(sock);
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
#if 1
  if((temp = find_timeout_by_ip(addr.sin_addr.s_addr))==NULL)
  {
    temp= malloc(sizeof(struct timer_elem));
    memset(temp, 0, sizeof(struct timer_elem));
    temp->dv = malloc(sizeof(distance_vector)*router_count);
    int index = find_router_by_ip(addr.sin_addr.s_addr);
    temp->ip = costs[index].ip;
    temp->port = costs[index].port_routing;
    temp->cost = costs[index].cost;
    temp->id = costs[index].id;
    gettimeofday(&(temp->timeout), NULL);
    temp->timeout.tv_sec += timeout;
    LOG("Next timeout router %d\n", temp->timeout.tv_sec);
//    list_push(in);
    list_insert_ordered(temp);
  }
  else
  {
    TAILQ_REMOVE(&timer_list, temp, next);
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
  LOG("Routing Updates from %d : Cost %d\n", temp->id, temp->cost);

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
    temp->dv[i].cost = router_cost;
    temp->dv[i].id = router_id;
//    int index = find_index_by_id(router_id, router_info);
    LOG("ID:%d Cost:%d\n", router_id, router_cost);
  
  }
  recalc_routing();
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
