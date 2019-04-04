#include <inttypes.h>
#include "sock-ads.h"
#include "logerr_info.h"

typedef struct ams_netid_port_type {
  uint8_t netID[6];
  uint8_t port_low;
  uint8_t port_high;
} ams_netid_port_type;


typedef struct {
  struct ams_tcp_header {
    uint8_t res0;
    uint8_t res1;
    uint8_t lenght_0;
    uint8_t lenght_1;
    uint8_t lenght_2;
    uint8_t lenght_3;
  } ams_tcp_header;
  struct ams_header {
    ams_netid_port_type target;
    ams_netid_port_type source;
    uint8_t cmdID_low;
    uint8_t cmdID_high;
    uint8_t stateFlags_low;
    uint8_t stateFlags_high;
    uint8_t lenght_0;
    uint8_t lenght_1;
    uint8_t lenght_2;
    uint8_t lenght_3;
    uint8_t errorCode_0;
    uint8_t errorCode_1;
    uint8_t errorCode_2;
    uint8_t errorCode_3;
    uint8_t invokeID_0;
    uint8_t invokeID_1;
    uint8_t invokeID_2;
    uint8_t invokeID_3;
    uint8_t data[256]; /* May be more or less */
  } ams_header;
} ads_req_type;


size_t handle_ads_request(int fd, char *buf, size_t len)
{
  ads_req_type *ads_req_p = (ads_req_type*)buf;
  LOGINFO7("%s/%s:%d len=%lu tcp_header=%x %x len=%x %x %x %x\n",
           __FILE__,__FUNCTION__, __LINE__,
           (unsigned long)len,
           ads_req_p->ams_tcp_header.res0,
           ads_req_p->ams_tcp_header.res1,
           ads_req_p->ams_tcp_header.lenght_0,
           ads_req_p->ams_tcp_header.lenght_1,
           ads_req_p->ams_tcp_header.lenght_2,
           ads_req_p->ams_tcp_header.lenght_3
           );
  LOGINFO7("%s/%s:%d ams_header target=%d.%d.%d.%d.%d.%d:%d source=%d.%d.%d.%d.%d.%d:%d\n",
           __FILE__,__FUNCTION__, __LINE__,
           ads_req_p->ams_header.target.netID[0],
           ads_req_p->ams_header.target.netID[1],
           ads_req_p->ams_header.target.netID[2],
           ads_req_p->ams_header.target.netID[3],
           ads_req_p->ams_header.target.netID[4],
           ads_req_p->ams_header.target.netID[5],
           ads_req_p->ams_header.target.port_low +
           ads_req_p->ams_header.target.port_high * 256,
           ads_req_p->ams_header.source.netID[0],
           ads_req_p->ams_header.source.netID[1],
           ads_req_p->ams_header.source.netID[2],
           ads_req_p->ams_header.source.netID[3],
           ads_req_p->ams_header.source.netID[4],
           ads_req_p->ams_header.source.netID[5],
           ads_req_p->ams_header.source.port_low +
           ads_req_p->ams_header.source.port_high * 256
           );
    LOGINFO7("%s/%s:%d ams_header cmd=%u.%u flags=%u.%u len=%u.%u.%u.%u err=%u.%u.%u.%u id=%u.%u.%u.%u\n",
           __FILE__,__FUNCTION__, __LINE__,
             ads_req_p->ams_header.cmdID_low,
             ads_req_p->ams_header.cmdID_high,
             ads_req_p->ams_header.stateFlags_low,
             ads_req_p->ams_header.stateFlags_high,
             ads_req_p->ams_header.lenght_0,
             ads_req_p->ams_header.lenght_1,
             ads_req_p->ams_header.lenght_2,
             ads_req_p->ams_header.lenght_3,
             ads_req_p->ams_header.errorCode_0,
             ads_req_p->ams_header.errorCode_1,
             ads_req_p->ams_header.errorCode_2,
             ads_req_p->ams_header.errorCode_3,
             ads_req_p->ams_header.invokeID_0,
             ads_req_p->ams_header.invokeID_1,
             ads_req_p->ams_header.invokeID_2,
             ads_req_p->ams_header.invokeID_3
           );
  return len;
}
