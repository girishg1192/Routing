#ifndef __FD_HEADER__
#define __FD_HEADER__
#include <fcntl.h>
#include <sys/select.h>

fd_set wait_fd;
fd_set temp;
int active_sockets;


void add_fd(int newfd);
void clear_fd(int delfd);
void reset_fd();
#endif
