#include <inttypes.h>
#include <string.h>
#include "sock-ads.h"
#include "sock-util.h"
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
  } ams_header;
  uint8_t data[256]; /* May be more or less */
} ads_req_type;

typedef struct {
  uint8_t indexGroup_0;
  uint8_t indexGroup_1;
  uint8_t indexGroup_2;
  uint8_t indexGroup_3;
  uint8_t indexOffset_0;
  uint8_t indexOffset_1;
  uint8_t indexOffset_2;
  uint8_t indexOffset_3;
  uint8_t lenght_0;
  uint8_t lenght_1;
  uint8_t lenght_2;
  uint8_t lenght_3;
} ads_read_req_type;

typedef struct {
  uint8_t result_0;
  uint8_t result_1;
  uint8_t result_2;
  uint8_t result_3;
  uint8_t major;
  uint8_t minor;
  uint8_t versionBuild_low;
  uint8_t versionBuild_high;
  char    deviceName[16];
} ADS_Read_Device_Info_rep_type;

static const uint16_t ADS_Read_Device_Info = 1;
static const uint16_t ADS_Read             = 2;

size_t handle_ads_request(int fd, char *buf, size_t len)
{
  ads_req_type *ads_req_p = (ads_req_type*)buf;
  uint16_t cmdId = ads_req_p->ams_header.cmdID_low +
                   (ads_req_p->ams_header.cmdID_high << 8);

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
  LOGINFO7("%s/%s:%d ams_header cmd=%u flags=%u len=%u err=%u id=%u\n",
           __FILE__,__FUNCTION__, __LINE__,
           cmdId,
           ads_req_p->ams_header.stateFlags_low +
           (ads_req_p->ams_header.stateFlags_high << 8),
           ads_req_p->ams_header.lenght_0 +
           (ads_req_p->ams_header.lenght_1 << 8) +
           (ads_req_p->ams_header.lenght_2 << 16) +
           (ads_req_p->ams_header.lenght_3 << 24),
           ads_req_p->ams_header.errorCode_0 +
           (ads_req_p->ams_header.errorCode_1 << 8) +
           (ads_req_p->ams_header.errorCode_2 << 16) +
           (ads_req_p->ams_header.errorCode_3 << 24),
           ads_req_p->ams_header.invokeID_0 +
           (ads_req_p->ams_header.invokeID_1 << 8) +
           (ads_req_p->ams_header.invokeID_2 << 16) +
           (ads_req_p->ams_header.invokeID_3 << 24)
           );


  if (cmdId == ADS_Read_Device_Info) {
    size_t send_len;
    send_len = sizeof(*ads_req_p) - sizeof(ads_req_p->data) +
      sizeof(ADS_Read_Device_Info_rep_type);
    ADS_Read_Device_Info_rep_type *ADS_Read_Device_Info_rep_p;
    ADS_Read_Device_Info_rep_p = (ADS_Read_Device_Info_rep_type *)&ads_req_p->data;
    memset(ADS_Read_Device_Info_rep_p, 0, sizeof(*ADS_Read_Device_Info_rep_p));

    ADS_Read_Device_Info_rep_p->major = 3;
    ADS_Read_Device_Info_rep_p->minor = 1;
    ADS_Read_Device_Info_rep_p->versionBuild_low = 10;
    ADS_Read_Device_Info_rep_p->versionBuild_high = 11;;
    strncpy(ADS_Read_Device_Info_rep_p->deviceName,
            "Simulator",
            sizeof(ADS_Read_Device_Info_rep_p->deviceName) - 1);
    ads_req_p->ams_header.stateFlags_low = 5;
    ads_req_p->ams_header.stateFlags_high = 0;
    ads_req_p->ams_header.lenght_0 = (uint8_t)(sizeof(ADS_Read_Device_Info_rep_type));
    ads_req_p->ams_header.lenght_1 = (uint8_t)(sizeof(ADS_Read_Device_Info_rep_type) << 8);
    ads_req_p->ams_header.lenght_2 = (uint8_t)(sizeof(ADS_Read_Device_Info_rep_type) << 16);
    ads_req_p->ams_header.lenght_3 = (uint8_t)(sizeof(ADS_Read_Device_Info_rep_type) << 24);

    send_to_socket(fd, buf, (unsigned)send_len);
    return len;
  } else if (cmdId == ADS_Read) {
    ads_read_req_type *ads_read_req_p = (ads_read_req_type *)&ads_req_p->data;
    uint32_t indexGroup = (uint32_t)ads_read_req_p->indexGroup_0 +
                          (ads_read_req_p->indexGroup_1 << 8) +
                          (ads_read_req_p->indexGroup_2 << 16) +
                          (ads_read_req_p->indexGroup_3 << 24);
    uint32_t indexOffset = (uint32_t)ads_read_req_p->indexOffset_0 +
                          (ads_read_req_p->indexOffset_1 << 8) +
                          (ads_read_req_p->indexOffset_2 << 16) +
                          (ads_read_req_p->indexOffset_3 << 24);
    uint32_t length = (uint32_t)ads_read_req_p->lenght_0 +
                      (ads_read_req_p->lenght_1 << 8) +
                      (ads_read_req_p->lenght_2 << 16) +
                      (ads_read_req_p->lenght_3 << 24);
    LOGINFO7("%s/%s:%d ADS_Readcmd indexGroup=0x%x indexOffset=%u length=%u \n",
             __FILE__,__FUNCTION__, __LINE__,
             indexGroup, indexOffset,length
             );

    LOGINFO7("%s/%s:%d ADS_Readcmd indexGroup=%u.%u.%u.%u indexOffset=%u.%u.%u.%u length=%u.%u.%u.%u\n",
             __FILE__,__FUNCTION__, __LINE__,
             ads_read_req_p->indexGroup_0,
             ads_read_req_p->indexGroup_1,
             ads_read_req_p->indexGroup_2,
             ads_read_req_p->indexGroup_3,
             ads_read_req_p->indexOffset_0,
             ads_read_req_p->indexOffset_1,
             ads_read_req_p->indexOffset_2,
             ads_read_req_p->indexOffset_3,
             ads_read_req_p->lenght_0,
             ads_read_req_p->lenght_1,
             ads_read_req_p->lenght_2,
             ads_read_req_p->lenght_3
             );
  }
  return 0; // len;
}
