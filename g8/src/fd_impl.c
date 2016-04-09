#include "fd_impl.h"
void add_fd(int newfd)
{
  FD_SET(newfd, &wait_fd);
  if(newfd >=active_sockets) 
    active_sockets = newfd+1;
}
void clear_fd(int oldfd)
{
  FD_CLR(oldfd, &wait_fd);
}
void reset_fd()
{
  FD_ZERO(&wait_fd);
  FD_ZERO(&temp);
}
