#include "impl_common.h"
#include <arpa/inet.h>
#include "fd_impl.h"


SOCKET start_router_data(int port);
SOCKET start_router_control(int port);
void router_data_receive(SOCKET sock);
void router_control_receive(SOCKET sock);
