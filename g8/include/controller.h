#include "impl_common.h"

SOCKET set_controller_listening_port(char* port_);
SOCKET controller_server_accept(SOCKET sock);
void control_message_receive(SOCKET sock);
struct control_message
{
  uint32_t ip;
  uint8_t code;
  uint8_t response_time;
  uint16_t length_data;
  char payload[100];
};

typedef struct control_message control_message;
