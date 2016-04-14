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
  SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
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

  err = listen(server_socket, MAX_NUMBER);
  check_error(err, "Listen");
  return server_socket;
}
