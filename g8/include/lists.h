#include "impl_common.h"
timer_elem *timeout_list_head, *timeout_list_tail;

void list_init();
void list_push(timer_elem *node);
timer_elem* list_peek();
timer_elem* list_pop();
uint32_t get_next_timeout();
void update_start();
void print_router_list();
uint32_t get_next_timeout();
uint32_t update_timeout();
