#include "lists.h"
static int init=0;
void list_init()
{
  timeout_list_head=NULL;
  timeout_list_tail=NULL;
}
void list_push(timer_elem *node)
{
  if(timeout_list_head==NULL)// && timeout_list_head == timeout_list_tail)
  {
    init = 1;
    timeout_list_head = node;
    timeout_list_tail = node;
    node->next = NULL;
    node->prev = NULL;
  }
  else
  {
    timeout_list_tail->next = node;
    node->prev = timeout_list_tail;
    node->next = NULL;
    timeout_list_tail = node;
  }
}
timer_elem* list_pop()
{
  if(timeout_list_head==NULL)
  {
    return NULL;
  }
  else
  {
    timer_elem* node = timeout_list_head;
    if(timeout_list_head->next == NULL)
    {
      timeout_list_head = NULL;
      timeout_list_tail = NULL;
      init=0;
    }
    else
    {
      timeout_list_head = timeout_list_head->next;
      timeout_list_head->prev = NULL;
    }
    return node;
  }
}
timer_elem* list_peek()
{
  if(timeout_list_head==NULL)
    return NULL;
  return timeout_list_head;
}
void update_start()
{
  if(init)
    return;
  list_init();
  timer_elem* updates = malloc(sizeof(timer_elem));
  memset(updates, 0, sizeof(timer_elem));
  updates->update = true;
  updates->failures = 0;
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  updates->timeout = curr_time.tv_sec + timeout;
  list_push(updates);
}
void print_router_list()
{
  char IP[INET_ADDRSTRLEN]; 
  for(int i=0; i<router_count; i++)
  {
    ip_readable(router_list[i].ip, IP);
    LOG("Router: %d: port %d %d\n Cost:%d IP:%s\n Neighbour %d\n",
        router_list[i].id, router_list[i].port_routing, router_list[i].port_data, 
        router_list[i].cost, IP, router_list[i].neighbour);
  }
}
uint32_t get_next_timeout()
{
  if(!init)
    return -1;
  timer_elem *check;
  check = list_peek();
  uint32_t ret=check->timeout;
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  //check->timeout = curr_time.tv_sec + timeout;
  ret = ret - curr_time.tv_sec;
  //list_push(check);
  return ret;
}
uint32_t update_timeout()
{
  timer_elem *update = list_pop();
  update->timeout = update->timeout + timeout;
  list_push(update);
  uint32_t ret = get_next_timeout();
  return ret;
}
