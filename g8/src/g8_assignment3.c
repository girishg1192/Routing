/**
 * @g8_assignment3
 * @author  Girish Gokul <g8@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
#include <sys/select.h>
#include <sys/time.h>
#include "controller.h"
#include "fd_impl.h"

int control_server_sock;
int control_sock;
int router_sock;
int router_crash = 0;

int main(int argc, char **argv)
{
	/*Start Here*/
  if(argc<=1)
    return 0;

  control_server_sock = set_controller_listening_port(argv[1]);
  active_sockets = control_server_sock;

  reset_fd();
  add_fd(control_server_sock);
  struct timeval tv;
  tv.tv_sec = 100;
  tv.tv_usec = 0;
  LOG("Control sock: %d\n", active_sockets);
  LOG("size:%x %d\n", 255, sizeof(int));
  temp = wait_fd;
  while(!router_crash && select(active_sockets, &temp, NULL, NULL, NULL))
  {
    if(FD_ISSET(control_server_sock, &temp))
    {
      control_sock = controller_server_accept(control_server_sock);
      add_fd(control_sock);
    }
    else if(FD_ISSET(control_sock, &temp))
    {
      control_message_receive(control_sock);
    }
    temp = wait_fd;
  }
  return 0;
}
