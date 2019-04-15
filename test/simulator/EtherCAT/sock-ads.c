#include <inttypes.h>
#include <string.h>
#include "ams.h"
#include "sock-ads.h"
#include "sock-util.h"
#include "logerr_info.h"
#include "indexer.h"

size_t handle_ads_request(int fd, char *buf, size_t len)
{
  ads_req_type *ads_req_p = (ads_req_type*)buf;
  uint16_t adsport = ads_req_p->ams_header.target.port_low +
    (ads_req_p->ams_header.target.port_high << 8);

  uint16_t cmdId = ads_req_p->ams_header.cmdID_low +
                   (ads_req_p->ams_header.cmdID_high << 8);
  uint32_t ams_lenght = ads_req_p->ams_tcp_header.lenght_0 +
    (ads_req_p->ams_tcp_header.lenght_1 << 8) +
    (ads_req_p->ams_tcp_header.lenght_2 << 16) +
    (ads_req_p->ams_tcp_header.lenght_3 <<24);

  LOGINFO7("%s/%s:%d len=%lu AMS tcp_header=%x %x ams_lenght=%u\n",
           __FILE__,__FUNCTION__, __LINE__,
           (unsigned long)len,
           ads_req_p->ams_tcp_header.res0,
           ads_req_p->ams_tcp_header.res1,
           ams_lenght);

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


  if (cmdId == ADS_READ_DEVICE_INFO) {
    const static char *const deviceName = "Simulator";
    uint32_t total_len_reply;

    total_len_reply = sizeof(*ads_req_p) - sizeof(ads_req_p->data) +
      sizeof(ADS_Read_Device_Info_rep_type);
    ADS_Read_Device_Info_rep_type *ADS_Read_Device_Info_rep_p;
    ADS_Read_Device_Info_rep_p = (ADS_Read_Device_Info_rep_type *)&ads_req_p->data;
    memset(ADS_Read_Device_Info_rep_p, 0, sizeof(*ADS_Read_Device_Info_rep_p));
    memset(ADS_Read_Device_Info_rep_p->deviceName, ' ',
           sizeof(*ADS_Read_Device_Info_rep_p->deviceName));

    ADS_Read_Device_Info_rep_p->major = 3;
    ADS_Read_Device_Info_rep_p->minor = 1;
    ADS_Read_Device_Info_rep_p->versionBuild_low = 10;
    ADS_Read_Device_Info_rep_p->versionBuild_high = 11;
    if (strlen(deviceName) < sizeof(*ADS_Read_Device_Info_rep_p->deviceName)) {
        memcpy(ADS_Read_Device_Info_rep_p->deviceName,
             deviceName, strlen(deviceName));
    }
    send_ams_reply(fd, ads_req_p, total_len_reply);
    return len;
  } else if (cmdId == ADS_READ) {
    ads_read_req_type *ads_read_req_p = (ads_read_req_type *)&ads_req_p->data;
    size_t total_len_reply;
    size_t payload_len;
    ADS_Read_rep_type *ADS_Read_rep_p;
    ADS_Read_rep_p = (ADS_Read_rep_type *)&ads_req_p->data;

    uint32_t indexGroup = (uint32_t)ads_read_req_p->indexGroup_0 +
                          (ads_read_req_p->indexGroup_1 << 8) +
                          (ads_read_req_p->indexGroup_2 << 16) +
                          (ads_read_req_p->indexGroup_3 << 24);
    uint32_t indexOffset = (uint32_t)ads_read_req_p->indexOffset_0 +
                          (ads_read_req_p->indexOffset_1 << 8) +
                          (ads_read_req_p->indexOffset_2 << 16) +
                          (ads_read_req_p->indexOffset_3 << 24);
    uint32_t len_in_PLC = (uint32_t)ads_read_req_p->lenght_0 +
      (ads_read_req_p->lenght_1 << 8) +
                      (ads_read_req_p->lenght_2 << 16) +
                      (ads_read_req_p->lenght_3 << 24);
    payload_len = sizeof(*ADS_Read_rep_p) -  sizeof(ADS_Read_rep_p->data) + len_in_PLC;
    total_len_reply = sizeof(*ads_req_p) - sizeof(ads_req_p->data) + payload_len;

    memset(ADS_Read_rep_p, 0, sizeof(*ADS_Read_rep_p));

    LOGINFO7("%s/%s:%d ADS_Readcmd indexGroup=0x%x indexOffset=%u len_in_PLC=%u payload_len=%u total_len_reply=%u\n",
             __FILE__,__FUNCTION__, __LINE__,
             indexGroup, indexOffset,len_in_PLC,
             (unsigned)payload_len, (unsigned)total_len_reply);
    ADS_Read_rep_p->lenght_0 = (uint8_t)(len_in_PLC);
    ADS_Read_rep_p->lenght_1 = (uint8_t)(len_in_PLC << 8);
    ADS_Read_rep_p->lenght_2 = (uint8_t)(len_in_PLC << 16);
    ADS_Read_rep_p->lenght_3 = (uint8_t)(len_in_PLC << 24);
    indexerHandlePLCcycle();
    (void)indexerHandleADS_ADR_getMemory(adsport,
                                         indexOffset,
                                         len_in_PLC,
                                         &ADS_Read_rep_p->data);
    send_ams_reply(fd, ads_req_p, total_len_reply);
    return len;
  } else if (cmdId == ADS_WRITE) {
    size_t total_len_reply = sizeof(*ads_req_p) -
      sizeof(ads_req_p->data) + sizeof(ADS_Write_rep_type);
    handleAMSwrite(fd, ads_req_p);
    indexerHandlePLCcycle();
    send_ams_reply(fd, ads_req_p, total_len_reply);
    return len;
  } else if (cmdId == ADS_READ_WRITE) {
    //size_t total_len_reply = sizeof(*ads_req_p) -
    //  sizeof(ads_req_p->data) + sizeof(ADS_ReadWrite_rep_type);
    handleAMSreadwrite(fd, ads_req_p);
#if 0
    indexerHandlePLCcycle();
    send_ams_reply(fd, ads_req_p, total_len_reply);
    return len;
#else
    RETURN_ERROR_OR_DIE(__LINE__,
                        "%s/%s:%d command not implemented =%u",
                        __FILE__, __FUNCTION__, __LINE__, cmdId);
#endif
  } else {
    RETURN_ERROR_OR_DIE(__LINE__,
                        "%s/%s:%d command not implemented =%u",
                        __FILE__, __FUNCTION__, __LINE__, cmdId);
  }


  return 0; // len;
}
