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
  //TODO handle actual routing and stuff
}
void router_control_receive(SOCKET sock)
{
  struct sockaddr_in addr;
  char buffer[1000];
  int fromlen = sizeof addr;
  int ret = recvfrom(sock, &buffer, sizeof(buffer), 0,
                  (struct sockaddr *)&addr, &fromlen);
  //TODO handle actual routing and stuff
}
