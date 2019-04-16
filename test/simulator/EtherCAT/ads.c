#include <string.h>
#include "logerr_info.h"
#include "indexer.h"
#include "sock-util.h"
#include "ads.h"

#define ADSIGRP_SYM_INFOBYNAMEEX 0xF009


void handleADSwrite(int fd, ams_hdr_type *ams_hdr_p)
{
  ADS_Write_req_type *ADS_Write_req_p = (ADS_Write_req_type *)ams_hdr_p;
  ADS_Write_rep_type *ADS_Write_rep_p = (ADS_Write_rep_type *)ams_hdr_p;
  uint16_t adsport = ams_hdr_p->target.port_low + (ams_hdr_p->target.port_high << 8);

  uint32_t indexGroup = (uint32_t)ADS_Write_req_p->indexGroup_0 +
                        (ADS_Write_req_p->indexGroup_1 << 8) +
                        (ADS_Write_req_p->indexGroup_2 << 16) +
                        (ADS_Write_req_p->indexGroup_3 << 24);
  uint32_t indexOffset = (uint32_t)ADS_Write_req_p->indexOffset_0 +
                        (ADS_Write_req_p->indexOffset_1 << 8) +
                        (ADS_Write_req_p->indexOffset_2 << 16) +
                        (ADS_Write_req_p->indexOffset_3 << 24);
  uint32_t len_in_PLC = (uint32_t)ADS_Write_req_p->lenght_0 +
                        (ADS_Write_req_p->lenght_1 << 8) +
                        (ADS_Write_req_p->lenght_2 << 16) +
                        (ADS_Write_req_p->lenght_3 << 24);

  memset(&ADS_Write_rep_p->response, 0, sizeof(ADS_Write_rep_p->response));

  LOGINFO7("%s/%s:%d ADS_Writecmd indexGroup=0x%x indexOffset=%u len_in_PLC=%u\n",
           __FILE__,__FUNCTION__, __LINE__,
           indexGroup, indexOffset,len_in_PLC);
  if (len_in_PLC == 2) {
    unsigned value;
    value = ADS_Write_req_p->data[0] +
      (ADS_Write_req_p->data[1] << 8);

    LOGINFO7("%s/%s:%d ADS_Writecmd data=0x%x 0x%x value=0x%x\n",
             __FILE__,__FUNCTION__, __LINE__,
             ADS_Write_req_p->data[0],
             ADS_Write_req_p->data[1],value);
    indexerHandleADS_ADR_putUInt(adsport,
                                 indexOffset,
                                 len_in_PLC,
                                 value);
  } else {
    (void)indexerHandleADS_ADR_setMemory(adsport,
                                         indexOffset,
                                         len_in_PLC,
                                         &ADS_Write_req_p->data);
  }
}

void handleADSreadwrite(int fd, ams_hdr_type *ams_hdr_p)
{
  ADS_ReadWrite_req_type *ADS_ReadWrite_req_p = (ADS_ReadWrite_req_type *)ams_hdr_p;
  ADS_ReadWrite_rep_type *ADS_ReadWrite_rep_p = (ADS_ReadWrite_rep_type *)ams_hdr_p;
  uint16_t adsport = ams_hdr_p->target.port_low + (ams_hdr_p->target.port_high << 8);

  uint32_t indexGroup = (uint32_t)ADS_ReadWrite_req_p->indexGroup_0 +
                        (ADS_ReadWrite_req_p->indexGroup_1 << 8) +
                        (ADS_ReadWrite_req_p->indexGroup_2 << 16) +
                        (ADS_ReadWrite_req_p->indexGroup_3 << 24);
  uint32_t indexOffset = (uint32_t)ADS_ReadWrite_req_p->indexOffset_0 +
                        (ADS_ReadWrite_req_p->indexOffset_1 << 8) +
                        (ADS_ReadWrite_req_p->indexOffset_2 << 16) +
                        (ADS_ReadWrite_req_p->indexOffset_3 << 24);
  uint32_t rd_len_in_PLC = (uint32_t)ADS_ReadWrite_req_p->rd_len_0 +
                           (ADS_ReadWrite_req_p->rd_len_1 << 8) +
                           (ADS_ReadWrite_req_p->rd_len_2 << 16) +
                           (ADS_ReadWrite_req_p->rd_len_3 << 24);

  uint32_t wr_len_in_PLC = (uint32_t)ADS_ReadWrite_req_p->wr_len_0 +
                           (ADS_ReadWrite_req_p->wr_len_1 << 8) +
                           (ADS_ReadWrite_req_p->wr_len_2 << 16) +
                           (ADS_ReadWrite_req_p->wr_len_3 << 24);

  memset(ADS_ReadWrite_rep_p, 0, sizeof(*ADS_ReadWrite_rep_p));

  LOGINFO7("%s/%s:%d ADS_ReadWritecmd indexGroup=0x%x indexOffset=%u rd_len_in_PLC=%u wr_len_in_PLC=%u\n",
           __FILE__,__FUNCTION__, __LINE__,
           indexGroup, indexOffset, rd_len_in_PLC, wr_len_in_PLC);
  if (indexGroup == ADSIGRP_SYM_INFOBYNAMEEX) {
    LOGINFO7("%s/%s:%d ADS_ReadWritecmd ADSIGRP_SYM_INFOBYNAMEEX (%s)\n",
           __FILE__,__FUNCTION__, __LINE__,
             ADS_ReadWrite_req_p->data
           );
  }


  if (wr_len_in_PLC == 2) {
    unsigned value;
    value = ADS_ReadWrite_req_p->data[0] +
      (ADS_ReadWrite_req_p->data[1] << 8);

    LOGINFO7("%s/%s:%d ADS_ReadWritecmd data=0x%x 0x%x value=0x%x\n",
             __FILE__,__FUNCTION__, __LINE__,
             ADS_ReadWrite_req_p->data[0],
             ADS_ReadWrite_req_p->data[1],value);
    indexerHandleADS_ADR_putUInt(adsport,
                                 indexOffset,
                                 wr_len_in_PLC,
                                 value);
  } else {
    (void)indexerHandleADS_ADR_setMemory(adsport,
                                         indexOffset,
                                         wr_len_in_PLC,
                                         &ADS_ReadWrite_req_p->data);
  }
}

void send_ams_reply(int fd, ams_hdr_type *ams_hdr_p, uint32_t total_len_reply)
{
  uint32_t ams_payload_len = total_len_reply -
    sizeof(ams_hdr_p->ams_tcp_header) -
    sizeof(ams_hdr_p->target) -
    sizeof(ams_hdr_p->source) - 16; /* TODO */

  LOGINFO7("%s/%s:%d total_len_reply=%u ams_payload_len=%u id=%u\n",
           __FILE__,__FUNCTION__, __LINE__,
           total_len_reply, ams_payload_len,
           ams_hdr_p->invokeID_0 +
           (ams_hdr_p->invokeID_1 << 8) +
           (ams_hdr_p->invokeID_2 << 16) +
           (ams_hdr_p->invokeID_3 << 24)
           );

  ams_hdr_p->stateFlags_low = 5;
  ams_hdr_p->stateFlags_high = 0;
  ams_hdr_p->lenght_0 = (uint8_t)ams_payload_len;
  ams_hdr_p->lenght_1 = (uint8_t)(ams_payload_len << 8);
  ams_hdr_p->lenght_2 = (uint8_t)(ams_payload_len << 16);
  ams_hdr_p->lenght_3 = (uint8_t)(ams_payload_len << 24);
  send_to_socket(fd, ams_hdr_p, total_len_reply);
}
