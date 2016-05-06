#include "lists.h"
static int init=0;
void list_init()
{
  timeout_list_head=NULL;
  timeout_list_tail=NULL;
}
#ifdef LIST_PRIM
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
void list_insert_ordered(timer_elem *node)
{
  timer_elem *a;
  if(timeout_list_head==NULL)
  {
    list_push(node);
    return;
  }
  for(a = timeout_list_head; a!=NULL; a=a->next)
  {
    if(timercmp(&(a->timeout), &(node->timeout), >))
    {
      if(a==timeout_list_head)
      {
        timeout_list_head=node;
        node->next = a;
        a->prev = node;
      }
      else
      {
        (a->prev)->next = node;
        node->prev = (a->prev);
        node->next = a;
        a->prev = node;
      }
      return;
    }
  }
  list_push(node);
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
void list_remove(timer_elem *temp)
{
  if(temp==NULL)
    return;
  if(temp==timeout_list_head)
  {
    list_pop();
  }
  else if(temp==timeout_list_tail)
  {
    timer_elem *prev = temp->prev;
    temp->prev = NULL;
    temp->next = NULL;
    prev->next = NULL;
    timeout_list_tail = prev;
  }
}
#endif
timer_elem* find_timeout_by_ip(uint32_t ip)
{
  timer_elem *temp;
  if(TAILQ_EMPTY(&timer_list))
    return NULL;
  TAILQ_FOREACH(temp, &timer_list, next)
  {
    if(temp->ip == ip)
      return temp;
  }
}
void update_start()
{
  if(init)
    return;
  TAILQ_INIT(&timer_list);
  timer_elem* updates = malloc(sizeof(timer_elem));
  memset(updates, 0, sizeof(timer_elem));
  updates->update = true;
  updates->failures = 0;
  gettimeofday(&(updates->timeout), NULL);
  updates->timeout.tv_sec+= timeout;
  TAILQ_INSERT_TAIL(&timer_list, updates, next);
  //TODO add timeout
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
struct timeval get_next_timeout()
{
  struct timeval null;
  memset(&null, 0, sizeof(struct timeval));

  timer_elem *check;
  check = TAILQ_FIRST(&timer_list);
  if(check == NULL)
  {
    return null;
  }
  return check->timeout;
}
void list_insert_ordered(timer_elem *node)
{
  if(TAILQ_EMPTY(&timer_list))
  {
    TAILQ_INSERT_HEAD(&timer_list, node, next);
    return;
  }
  struct timer_elem *temp;
  TAILQ_FOREACH(temp, &timer_list, next)
  {
    if(timercmp(&(temp->timeout), &(node->timeout), >))
    {
      TAILQ_INSERT_BEFORE(temp, node, next);
      return;
    }
  }
  TAILQ_INSERT_TAIL(&timer_list, node, next);
}
struct timeval update_timeout()
{
  if(!TAILQ_EMPTY(&timer_list))
  {
    timer_elem *update = TAILQ_FIRST(&timer_list);
    TAILQ_REMOVE(&timer_list, update, next);
    update->timeout.tv_sec += timeout;
//    list_push(update);
    list_insert_ordered(update);
  }
  return get_next_timeout();
}

file_stats* find_file_transfer_id(uint8_t tfer_id)
{
  file_stats *res;
  if(stats_list==NULL)
    return stats_list;
  for(res = stats_list; res!=NULL; res=res->next)
  {
    if(res->transfer_id == tfer_id)
      break;
  }
  return res;
}
void insert_file(file_stats *new)
{
  if(stats_list==NULL)
  {
    stats_list = new;
    new->next=NULL;
    return;
  }
  new->next = stats_list;
  stats_list = new;
}
