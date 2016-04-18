#include "impl_common.h"
#include "fd_impl.h"

SOCKET controller_server_accept(SOCKET sock);
int control_message_receive(SOCKET sock);
struct control_message
{
  uint32_t ip;
  uint8_t code;
  uint8_t response_time;
  uint16_t length_data;
};
enum CONTROL_CODE
{
  AUTHOR,
  INIT,
  ROUTING_TABLE,
  UPDATE,
  CRASH,
  SENDFILE,
  SENDFILE_STATS ,
  LAST_DATA_PACKET,
  PENULTIMATE_DATA_PACKET
};

typedef struct control_message control_message;
