#include "controller.h"

void send_author(SOCKET sock, control_message response);
void init_vectors(SOCKET sock, control_message header);
void print_buffer(char *data, int ret);
int get_peer_from_socket(SOCKET sock);
void ip_readable(uint32_t ip, char *IP);

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
void control_message_receive(SOCKET sock)
{
  control_message message;
  int ret = recv(sock, &message, sizeof(message), 0);
  if(ret<=0)
  {
    LOG("Close connection");
    clear_fd(sock);
    close(sock);
    return;
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
                 break;
  }
  //TODO cases for message.code
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
  router_list = malloc(sizeof(router_info)*router_count);
#endif

  for(int i=0; i<router_count; i++)
  {
#ifdef ARRAY_ROUTER
    LOG("Router %d init\n", i);
    memcpy(&router_list[i], data, sizeof(router_info));
#else
    list_elem *init_router_elem = malloc(sizeof(list_elem));
    memcpy(&init_router_elem->router_inf, data, sizeof(router_info));
#endif
    data = data+sizeof(router_info);
    //TODO add to list
    char IP[INET_ADDRSTRLEN];
    ip_readable(router_list[i].ip, IP);
    router_list[i].id = ntohs(router_list[i].id);
    router_list[i].port_routing = ntohs(router_list[i].port_routing);
    router_list[i].port_data = ntohs(router_list[i].port_data);
    router_list[i].cost = ntohs(router_list[i].cost);
    if(router_list[i].cost == 0)
    {
      router_data = router_list[i].port_data;
      router_data_sock = create_socket_on_port(router_data, SOCK_DGRAM);
      add_fd(router_data);

      router_control = router_list[i].port_routing;
      router_control_sock = create_socket_on_port(router_control, SOCK_DGRAM);
      add_fd(router_control);
    }
    LOG("Router: %d: port %d %d\n cost:%d IP:%s\n",
        router_list[i].id, router_list[i].port_routing, router_list[i].port_data, 
        router_list[i].cost, IP);

  }
  header.ip = get_peer_from_socket(sock);
  header.response_time = 0;
  header.length_data = 0;
  send(sock, &header, sizeof(header), 0);

}
int get_peer_from_socket(SOCKET sock)
{
  struct sockaddr_storage in;
  socklen_t len = sizeof(in);
  getpeername(sock, (struct sockaddr*)&in, &len);
  return (((struct sockaddr_in*)&in)->sin_addr).s_addr;
}
void print_buffer(char *data, int ret)
{
  for(int i=0; i<ret; i++)
  {
    if(i%4==0)LOG("\n");
    LOG("%02x ", data[i]);
  }
}
void ip_readable(uint32_t ip, char *IP)
{
  inet_ntop(AF_INET, &ip, IP, INET_ADDRSTRLEN);
}

