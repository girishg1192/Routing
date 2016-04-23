#include "controller.h"
#include <unistd.h>
#include "lists.h"

void send_author(SOCKET sock, control_message response);
void init_vectors(SOCKET sock, control_message header);
void print_buffer(char *data, int ret);
int get_peer_from_socket(SOCKET sock);
void ip_readable(uint32_t ip, char *IP);
void crash_send(SOCKET sock, control_message response);
void update_router(SOCKET sock, control_message response);
void routing_table_send(SOCKET sock, control_message message);
void start_sendfile(SOCKET sock, control_message message);

extern int router_data, router_control;
extern int router_data_sock, router_control_sock;

SOCKET controller_server_accept(SOCKET sock)
{
  struct sockaddr_storage client_addr;
  socklen_t size_address = sizeof(struct sockaddr_storage);
  LOG("server_wait\n");
  int newfd = accept(sock, (struct sockaddr *) &client_addr, &size_address);
  int port = ((struct sockaddr_in *)&client_addr)->sin_port;
  LOG("ControllerAccept: %d\n", port);
  return newfd;
}
int control_message_receive(SOCKET sock)
{
  control_message message;
  int ret = recv(sock, &message, sizeof(message), 0);
  if(ret<=0)
  {
    LOG("Close connection");
    clear_fd(sock);
    close(sock);
    return -1;
  }
  char IP[INET_ADDRSTRLEN];
  ip_readable(message.ip, IP);
  LOG("Control: Sender %s\n", IP);
  LOG("Control: %x\tMessage Code:%x \n", ntohl(message.ip), message.code);
  LOG("Control length: %d\n", ntohs(message.length_data));
  switch(message.code)
  {
    case AUTHOR: send_author(sock, message);
                 break;
    case INIT:
                 init_vectors(sock, message);
                 return INIT;
                 break;
    case ROUTING_TABLE:
                 routing_table_send(sock, message);
                 return ROUTING_TABLE;
                 break;
    case UPDATE:
                 update_router(sock, message);
                 return UPDATE;
                 break;
    case CRASH:
                 crash_send(sock, message);
                 return CRASH;
                 break;
    case SENDFILE:
                 start_sendfile(sock, message);
                 return SENDFILE;
                 break;
  }
  //TODO receive args?
}
void send_author(SOCKET sock, control_message response)
{
  char author[] = "I, g8, have read and understood the course academic integrity policy.";

  response.ip = get_peer_from_socket(sock);
  uint16_t payload_length = sizeof(author);

  response.response_time = 0;
  response.length_data = htons(payload_length);
  LOG("payload length %x %x\n", htons(payload_length), payload_length);
  char *res = malloc(sizeof(response)+sizeof(author));
  char *copy = res;
  memcpy(copy, &response, sizeof(response));
  LOG("Sending Author %s\n", author);
  copy = copy+sizeof(response);
  memcpy(copy, author, strlen(author));

  send(sock, res, sizeof(author)+sizeof(response), 0);
  free(res);
}
void init_vectors(SOCKET sock, control_message header)
{
  char *data = malloc(header.length_data);
  char *cleanup = data;
  int ret = recv(sock, data, header.length_data, 0);
  LOG("INIT received %d\n", ret);
  print_buffer(data, ret);
  //Parse common info -> router count and update interval
  memcpy(&router_count, data, sizeof(uint16_t));
  router_count = ntohs(router_count);
  data = data+sizeof(uint16_t);
  memcpy(&timeout, data, sizeof(uint16_t));
  timeout = ntohs(timeout);
  data = data+sizeof(uint16_t);
  ret = ret-2*sizeof(uint16_t);
  LOG("\n");
  LOG("\nRouter Count %x data %d\nTimeout %d\n", router_count, ret/12, timeout);

#ifdef ARRAY_ROUTER
  int mem_size = sizeof(router_info)*router_count;
  router_list = malloc(mem_size);
  memset(router_list, 0, mem_size);
#endif

  for(int i=0; i<router_count; i++)
  {
#ifdef ARRAY_ROUTER
    LOG("Router %d init\n", i);
    memcpy(&router_list[i], data, ROUTER_INFO_HEADER);
#else
    list_elem *init_router_elem = malloc(sizeof(list_elem));
    memcpy(&init_router_elem->router_inf, data, sizeof(router_info));
#endif
    data = data+ROUTER_INFO_HEADER;
    //TODO add to list
    char IP[INET_ADDRSTRLEN];
    ip_readable(router_list[i].ip, IP);
    router_list[i].id = ntohs(router_list[i].id);
    router_list[i].port_routing = ntohs(router_list[i].port_routing);
    router_list[i].port_data = ntohs(router_list[i].port_data);
    router_list[i].cost = ntohs(router_list[i].cost);
    router_list[i].nexthop_id = router_list[i].id;
    router_list[i].nexthop_index = i;
    router_list[i].neighbour = false;
    if(router_list[i].cost!=UINT16_T_MAX && router_list[i].cost!=0)
      router_list[i].neighbour = true;
    if(router_list[i].cost == 0)
    {
      local_ip = router_list[i].ip;
      router_data = router_list[i].port_data;
      router_data_sock = create_socket_on_port(router_data, SOCK_STREAM);
      add_fd(router_data_sock);

      router_control = router_list[i].port_routing;
      router_control_sock = create_socket_on_port(router_control, SOCK_DGRAM);
      add_fd(router_control_sock);
    }
    LOG("Router: %d: port %d %d\n cost:%d IP:%s\n",
        router_list[i].id, router_list[i].port_routing, router_list[i].port_data, 
        router_list[i].cost, IP);
  }
  header.ip = get_peer_from_socket(sock);
  header.response_time = 0;
  header.length_data = 0;
  send(sock, &header, sizeof(header), 0);
  free(cleanup);
}
int get_peer_from_socket(SOCKET sock)
{
  struct sockaddr_storage in;
  socklen_t len = sizeof(in);
  getpeername(sock, (struct sockaddr*)&in, &len);
  return (((struct sockaddr_in*)&in)->sin_addr).s_addr;
}
void crash_send(SOCKET sock, control_message response)
{
  response.ip = get_peer_from_socket(sock);
  response.response_time = 0;
  response.length_data = 0;
  send(sock, &response, sizeof(response), 0);
}
void update_router(SOCKET sock, control_message response)
{
  char *data = malloc(response.length_data);
  char *cleanup;
  int ret = recv(sock, data, response.length_data, 0);
  uint16_t router_id, router_cost;
  memcpy(&router_id, data, sizeof(uint16_t));
  router_id = ntohs(router_id);
  data = data+sizeof(uint16_t);
  memcpy(&router_cost, data, sizeof(uint16_t));
  router_cost = ntohs(router_cost);
  data = data+sizeof(uint16_t);
  int i = find_index_by_id(router_id);
  router_list[i].cost = router_cost;
  LOG("Update router %d %d %x to %d", router_list[i].id, 
      router_list[i].port_routing, router_list[i].ip, router_cost);

  response.ip = get_peer_from_socket(sock);
  response.response_time = 0;
  response.length_data = 0;
  send(sock, &response, sizeof(response), 0);
  free(cleanup);
}
void routing_table_send(SOCKET sock, control_message message)
{
  int size = sizeof(control_message) + router_count*ROUTING_TABLE_UPDATE;
  char *table = malloc(size);
  char *result = table;
  memset(table, 0 , size);
  //table+= sizeof(control_message);  //skip header
  //or Not
  message.ip = get_peer_from_socket(sock);
  message.response_time = 0;
  message.length_data = htons(router_count*ROUTING_TABLE_UPDATE);
  memcpy(table, &message, sizeof(control_message));
  table+= sizeof(control_message);
  //TODO send crashed nodes?
  for(int i=0; i<router_count; i++)
  {
    uint16_t temp = htons(router_list[i].id);
    memcpy(table, &temp, sizeof(uint16_t));
    table+= sizeof(uint32_t);
    temp = htons(router_list[i].nexthop_id);
    memcpy(table, &temp, sizeof(uint16_t));
    table+= sizeof(uint16_t);
    temp = htons(router_list[i].cost);
    memcpy(table, &temp, sizeof(uint16_t));
    table+= sizeof(uint16_t);
  }
  send(sock, result, size, 0);
  free(result);
}

void start_sendfile(SOCKET sock, control_message message)
{
  char *temp = malloc(message.length_data);
  char *cleanup = temp;
  int ret = recv(sock, temp, message.length_data, 0);
  data_packet file_packet;
  memset(&file_packet, 0, sizeof(data_packet));
  memcpy(&file_packet, temp, DATA_CONTROLLER_HEADER_SIZE);
  temp = temp+ DATA_CONTROLLER_HEADER_SIZE;
  int xfer_id = file_packet.transfer_id;
  file_packet.transfer_id = file_packet.ttl;
  file_packet.ttl = xfer_id;
  if(file_packet.ttl==0);
    //TODO do not send packet
  file_packet.seq_no = ntohs(file_packet.seq_no);
  LOG("Sendfile header received %d ", DATA_CONTROLLER_HEADER_SIZE);
  int filename_length = message.length_data - DATA_CONTROLLER_HEADER_SIZE;
  char *file_name = malloc(filename_length);
  memcpy(file_name, temp, filename_length);
  LOG("Filename %s\n", file_name);

  //Open connection to nexthop
  int nexthop_index = find_nexthop_by_ip(file_packet.dest_ip);
  LOG("Nexthop %d port %d\n", router_list[nexthop_index].id, router_list[nexthop_index].port_data);
  SOCKET nexthop_sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in in;
  bzero(&in, sizeof(in));
  in.sin_family = AF_INET;
  in.sin_addr.s_addr = router_list[nexthop_index].ip;
  in.sin_port = htons(router_list[nexthop_index].port_data);
  ret = connect(nexthop_sock, (struct sockaddr *)&in, sizeof(in));
  check_error(ret, "Sendfile connect");

  FILE *fp = fopen(file_name, "rb");
  int eof=0;
  char buffer[1024];
  if(fp==NULL)
  {
    perror("file open");
    goto error;
  }
  while(!eof)
  {
    int bytes_sent = fread(buffer, CHUNK_SIZE, 1, fp);
    LOG("bytes read? %d %d\n", bytes_sent, CHUNK_SIZE);
    LOG("%s", buffer);
    if(feof(fp))
    {
      LOG("End of file\n");
      file_packet.fin = 1;
      eof=1;
    }
    LOG("sendfile %d\n", sizeof(data_packet));
    memcpy(file_packet.payload, buffer, CHUNK_SIZE);
    //TODO keep stats
    send(nexthop_sock, &file_packet, sizeof(data_packet), 0);
  }
  error:
  message.ip = get_peer_from_socket(sock);
  message.response_time = 0;
  message.length_data = 0;
  send(sock, &message, sizeof(message), 0);
  free(cleanup);
  free(file_name);
}
