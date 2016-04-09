#include "controller.h"

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
  int err = bind(server_socket, servinfo->ai_addr, servinfo->ai_addrlen);
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
  if(!ret)
  {
    clear_fd(sock);
    close(sock);
  }
  LOG("Control: %x\t %d", message.ip, message.code);
}
