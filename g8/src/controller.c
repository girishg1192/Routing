#include "controller.h"

void send_author(SOCKET sock, control_message response);
void init_vectors();

SOCKET set_controller_listening_port(char* port_)
{
  char *end;
  int port = strtol(port_, &end, 10);
  LOG("ControlPort: %d\n", port);
  struct addrinfo *servinfo;
  SOCKET server_socket = create_socket(&servinfo, NULL, port_);
  int yes =1;

  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("Socket already in use, setsockopt failed");
    exit(1);
  }
  int final_port = ntohs(((struct sockaddr_in *)servinfo->ai_addr)->sin_port);
  LOG("ControlPort: listen port %d\n", final_port);

  struct sockaddr_in in;
  bzero(&in, sizeof(in));
  int hport = htons(port);
  in.sin_family = AF_INET;
  in.sin_addr.s_addr = htonl(INADDR_ANY);
  in.sin_port = hport;

  int err = bind(server_socket, (struct sockaddr *) &in, sizeof(in));
  //int err = bind(server_socket, servinfo->ai_addr, servinfo->ai_addrlen);
  check_error(err, "Bind");

  err = listen(server_socket, MAX_NUMBER);
  check_error(err, "Listen");
  freeaddrinfo(servinfo);
  return server_socket;
}
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
  inet_ntop(AF_INET, &(message.ip), IP, sizeof(IP));
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
  response.length_data = htons(payload_length)>>8;
  LOG("payload length %x %x\n", htons(payload_length)>>8, payload_length);
  char *res = malloc(sizeof(response)+sizeof(author));
  char *copy = res;
  memcpy(copy, &response, sizeof(response));
  copy = copy+sizeof(response);
  memcpy(copy, author, strlen(author));

  LOG("Sending Author %s\n", author);
  send(sock, res, sizeof(author)+sizeof(response), 0);
  free(res);
}
void init_vectors(SOCKET sock, control_message header)
{
  char *data = malloc(header.length_data);
  int ret = recv(sock, data, header.length_data, 0);
  LOG("INIT received %d\n", ret);
  for(int i=0; i<ret; i++)
  {
    if(i%4==0)LOG("\n");
    LOG("%02x ", data[i]);
  }

  //TODO
  ;
}
int get_peer_from_socket(SOCKET sock)
{
  struct sockaddr_storage in;
  socklen_t len = sizeof(in);
  getpeername(sock, (struct sockaddr*)&in, &len);
  return (((struct sockaddr_in*)&in)->sin_addr).s_addr;
}
