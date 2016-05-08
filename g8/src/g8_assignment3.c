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
#include "controller.h"
#include "router.h"
#include "lists.h"
#include "fd_impl.h"

SOCKET control_server_sock;
extern int router_data, router_control;
extern int router_data_sock, router_control_sock;
int control_sock;
int router_sock;
int router_crash = 0;
struct timeval check_timeout();

int main(int argc, char **argv)
{
	/*Start Here*/
  if(argc<=1)
    return 0;
  char *end;
  int port = strtol(argv[1], &end, 10);
  control_server_sock = create_socket_on_port(port, SOCK_STREAM);
  active_sockets = control_server_sock;

  reset_fd();
  add_fd(control_server_sock);
  struct timeval tv;
  tv.tv_sec = 100;
  tv.tv_usec = 0;
  LOG("Control sock: %d\n", active_sockets);
  router_data_sock = 1000;
  router_control_sock = 1000;
  temp = wait_fd;
  memset(&last_packet, 0, sizeof(data_packet));
  memset(&not_last, 0, sizeof(data_packet));
  int ret;
  while(!router_crash && (ret=select(active_sockets, &temp, NULL, NULL, &tv))>=0)
  {
    if(ret==0)
    {
      struct timeval curr_time;
      tv = check_timeout();

      //Next timeout calculation
      tv = check_and_set_timer(tv);
    }
    if(FD_ISSET(control_server_sock, &temp))
    {
      control_sock = controller_server_accept(control_server_sock);
      add_fd(control_sock);
      FD_CLR(control_server_sock, &temp);
    }
    if(FD_ISSET(control_sock, &temp))
    {
      int code = control_message_receive(control_sock);
      if(code == 1)
      {
        //TODO move to function
        update_start();
        router_send_updates();
        tv.tv_sec = timeout;
      }
      else if(code==4)
        router_crash = 1;

      FD_CLR(control_sock ,&temp);

      if(code == -1)
        control_sock = 1000; 
    }
    if(FD_ISSET(router_data_sock, &temp))
    {
      //Router data is TCP
      int sockfd = controller_server_accept(router_data_sock);
      LOG("Incoming Router connection %d\n", sockfd);
//      router_data_receive(sockfd);
      add_fd(sockfd);
      FD_CLR(router_data_sock ,&temp);
    }
    if(FD_ISSET(router_control_sock, &temp))
    {
      router_control_receive(router_control_sock);
      //get next timeout after control receive
      tv = get_next_timeout();
      struct timeval curr_time;
      gettimeofday(&curr_time, NULL);
      LOG("Next timeout %d curr %d\n\n", tv.tv_sec, curr_time.tv_sec);
      if(tv.tv_sec<=curr_time.tv_sec)
      {
        tv=check_timeout();
        LOG("Reset timeout to tv %d", tv.tv_sec);
      }
      timersub(&tv, &curr_time, &tv);
      FD_CLR(router_control_sock ,&temp);
    }
    for(int fd = 2; fd<=active_sockets; fd++)
    {
      if(FD_ISSET(fd, &temp))
      {
//        LOG("Incoming router Data %d\n", fd);
        router_data_receive(fd);
        //TODO clear FDs after reading status
      }
    }
    temp = wait_fd;
    LOG("Ports %d %d %d\n", control_sock, router_data_sock, router_control_sock);
    struct timeval curr_time;
    gettimeofday(&curr_time, NULL);
    LOG("Time-> %ld timeout->%d\n", curr_time.tv_sec, tv.tv_sec);
  }
  return 0;
}
struct timeval check_timeout()
{
  struct timeval curr_time, tv;
  do
  {
    LOG("Recalculate\n");
    timer_elem *curr = TAILQ_FIRST(&timer_list);
    int failed=0;
    if(curr!=NULL && curr->update)
      router_send_updates();
    else
    {
      timer_elem *curr = TAILQ_FIRST(&timer_list);
      //Update failure for neighbours
      LOG("Something failed %x %d %d times\n", curr->ip, curr->port,
          curr->failures);
      curr->failures++;
      if(curr->failures==3)
      {
        LOG("Node crashed\n");
        int failed_index = find_router_by_ip(curr->ip);
        LOG("Router %d %d Failed", router_list[failed_index].id, 
            router_list[failed_index].cost);
        router_list[failed_index].cost = UINT16_T_MAX;
        costs[failed_index].cost = UINT16_T_MAX;
        TAILQ_REMOVE(&timer_list, curr, next);
        failed=1;
      }
    }
    //Push back to queue
    gettimeofday(&curr_time, NULL);
    if(!failed)
      tv = update_timeout();
    else
      tv = get_next_timeout();
  }while(curr_time.tv_sec >= tv.tv_sec);
  return tv;
}
