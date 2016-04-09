#include "impl_common.h"

void fill_addrinfo(struct addrinfo *info)
{
  if(info==NULL)
    exit(1);

  memset(info, 0, sizeof(struct addrinfo));
  info->ai_family = AF_INET;
  info->ai_socktype = SOCK_STREAM;
}
SOCKET create_socket(struct addrinfo **servinfo_, char* host, char *port)
{
  struct addrinfo *servinfo = *servinfo_;
  struct addrinfo hints;
  fill_addrinfo(&hints);
  getaddrinfo(host, port, &hints, &servinfo);
  int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  *servinfo_ = servinfo;
  return sockfd;
}
void check_error(int err, char* func)
{
  if(err)
  {
    LOG("%s failed = %d\n",func, err);
    perror("Failed!");
  }
}
