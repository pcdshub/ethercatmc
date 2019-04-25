#include <stdlib.h>
#include "EthercatMCController.h"
#include "EthercatMCADSdefs.h"
#include <asynOctetSyncIO.h>

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

#define DEFAULT_CONTROLLER_TIMEOUT 2.0

static uint32_t invokeID;

#define EthercatMChexdump(pasynUser, tracelevel, help_txt, bufptr, buflen)\
{\
  const void* buf = (const void*)bufptr;\
  int len = (int)buflen;\
  uint8_t *data = (uint8_t *)buf;\
  int count;\
  unsigned pos = 0;\
  while (len > 0) {\
    struct {\
      char asc_txt[8];\
      char space[2];\
      char hex_txt[8][3];\
      char nul;\
    } print_buf;\
    memset(&print_buf, ' ', sizeof(print_buf));\
    print_buf.nul = '\0';\
    for (count = 0; count < 8; count++) {\
      if (count < len) {\
        unsigned char c = (unsigned char)data[count];\
        if (c > 0x32 && c < 0x7F)\
          print_buf.asc_txt[count] = c;\
        else\
          print_buf.asc_txt[count] = '.';\
        snprintf((char*)&print_buf.hex_txt[count],\
                 sizeof(print_buf.hex_txt[count]),\
                 "%02x", c);\
        /* Replace NUL with ' ' after snprintf */\
        print_buf.hex_txt[count][2] = ' ';\
      }\
    }\
    asynPrint(pasynUser, tracelevel,\
              "%s %s [%02x]%s\n",\
              modNamEMC, help_txt, pos, (char*)&print_buf);\
    len -= 8;\
    data += 8;\
    pos += 8;\
  }\
}\


#define EthercatMCamsdump(pasynUser, tracelevel, help_txt, ams_headdr_p)\
{\
  const ams_hdr_type *ams_hdr_p = (const ams_hdr_type *)(ams_headdr_p);\
  uint32_t ams_tcp_hdr_len = ams_hdr_p->ams_tcp_hdr.length_0 +\
    (ams_hdr_p->ams_tcp_hdr.length_1 << 8) +\
    (ams_hdr_p->ams_tcp_hdr.length_2 << 16) +\
    (ams_hdr_p->ams_tcp_hdr.length_3 <<24);\
    uint32_t ams_lenght = ams_hdr_p->length_0 +\
      (ams_hdr_p->length_1 << 8) +\
      (ams_hdr_p->length_2 << 16) +\
      (ams_hdr_p->length_3 << 24);\
    uint32_t ams_errorCode = ams_hdr_p->errorCode_0 +\
      (ams_hdr_p->errorCode_1 << 8) +\
      (ams_hdr_p->errorCode_2 << 16) +\
      (ams_hdr_p->errorCode_3 << 24);\
    uint32_t ams_invokeID = ams_hdr_p->invokeID_0 +\
      (ams_hdr_p->invokeID_1 << 8) +\
      (ams_hdr_p->invokeID_2 << 16) +\
      (ams_hdr_p->invokeID_3 << 24);\
  asynPrint(pasynUser, tracelevel,\
            "ams_tcp_hdr_len=%u ams target=%d.%d.%d.%d.%d.%d:%d source=%d.%d.%d.%d.%d.%d:%d\n",\
            ams_tcp_hdr_len,\
            ams_hdr_p->target.netID[0], ams_hdr_p->target.netID[1],\
            ams_hdr_p->target.netID[2], ams_hdr_p->target.netID[3],\
            ams_hdr_p->target.netID[4], ams_hdr_p->target.netID[5],\
            ams_hdr_p->target.port_low + (ams_hdr_p->target.port_high << 8),\
            ams_hdr_p->source.netID[0],  ams_hdr_p->source.netID[1],\
            ams_hdr_p->source.netID[2],  ams_hdr_p->source.netID[3],\
            ams_hdr_p->source.netID[4],  ams_hdr_p->source.netID[5],\
            ams_hdr_p->source.port_low + (ams_hdr_p->source.port_high << 8)\
            );\
  asynPrint(pasynUser, tracelevel,\
            "ams_hdr cmd=%u flags=%u ams_len=%u ams_err=%u id=%u\n",\
            ams_hdr_p->cmdID_low + (ams_hdr_p->cmdID_high <<8),\
            ams_hdr_p->stateFlags_low + (ams_hdr_p->stateFlags_high << 8),\
            ams_lenght, ams_errorCode, ams_invokeID);\
}\


extern "C"
asynStatus writeReadBinaryOnErrorDisconnect_C(asynUser *pasynUser,
                                              const char *outdata, size_t outlen,
                                              char *indata, size_t inlen,
                                              size_t *pnwrite, size_t *pnread,
                                              int *peomReason)
{
  char old_InputEos[10];
  int old_InputEosLen = 0;
  char old_OutputEos[10];
  int old_OutputEosLen = 0;
  asynStatus status;
  status = pasynOctetSyncIO->getInputEos(pasynUser,
                                         &old_InputEos[0],
                                         (int)sizeof(old_InputEos),
                                         &old_InputEosLen);
  if (status) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n", modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_Eos;
  }
  status = pasynOctetSyncIO->getOutputEos(pasynUser,
                                          &old_OutputEos[0],
                                          (int)sizeof(old_OutputEos),
                                          &old_OutputEosLen);
  if (status) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n", modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_Eos;
  }
  status = pasynOctetSyncIO->setInputEos(pasynUser, "", 0);
  if (status) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n", modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_Eos;
  }
  status = pasynOctetSyncIO->setOutputEos(pasynUser, "", 0);
  if (status) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n",
              modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_Eos;
  }
  status = pasynOctetSyncIO->writeRead(pasynUser, outdata, outlen,
                                       indata, inlen,
                                       DEFAULT_CONTROLLER_TIMEOUT,
                                       pnwrite, pnread, peomReason);
  if ((status == asynTimeout) ||
      (!status && !*pnread && (*peomReason & ASYN_EOM_END))) {
    int eomReason = *peomReason;
    EthercatMChexdump(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER, "OUT",
                      outdata, outlen);
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s calling disconnect_C outlen=%lu nwrite=%lu nread=%lu timeout=%f eomReason=%x (%s%s%s) status=%d\n",
              modNamEMC,
              (unsigned long)outlen,
              (unsigned long)*pnwrite, (unsigned long)*pnread,
              DEFAULT_CONTROLLER_TIMEOUT,
              eomReason,
              eomReason & ASYN_EOM_CNT ? "CNT" : "",
              eomReason & ASYN_EOM_EOS ? "EOS" : "",
              eomReason & ASYN_EOM_END ? "END" : "",
              status);
    disconnect_C(pasynUser);
    status = asynError; /* TimeOut -> Error */
  } else {
    asynPrint(pasynUser, ASYN_TRACE_INFO,
              "%spasynOctetSyncIO->writeRead outlen=%u inlen=%u nread=%lu, status=%s (%d)\n",
              modNamEMC,  (unsigned)outlen,
              (unsigned)inlen,
              (unsigned long)*pnread,
              pasynManager->strStatus(status), status);
  }

restore_Eos:
  {
    asynStatus cmdStatus;
    cmdStatus = pasynOctetSyncIO->setInputEos(pasynUser,
                                              old_InputEos, old_InputEosLen);
    if (cmdStatus) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%scmdStatus=%s (%d)\n", modNamEMC,
                pasynManager->strStatus(cmdStatus), (int)cmdStatus);
    }
    cmdStatus = pasynOctetSyncIO->setOutputEos(pasynUser,
                                               old_OutputEos,
                                               old_OutputEosLen);
    if (cmdStatus) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%scmdStatus=%s (%d)\n", modNamEMC,
                pasynManager->strStatus(cmdStatus), (int)cmdStatus);
    }
  }
  return status;
}

asynStatus EthercatMCController::writeWriteReadAds(asynUser *pasynUser,
                                                   ams_hdr_type *ams_hdr_p, size_t outlen,
                                                   uint32_t invokeID,
                                                   uint32_t ads_cmdID,
                                                   void *indata, size_t inlen,
                                                   size_t *pnread)
{
  size_t nwrite = 0;
  int eomReason = 0;
  uint32_t ams_payload_len = outlen - sizeof(ams_hdr_p->ams_tcp_hdr);
  uint32_t ads_len = outlen - sizeof(*ams_hdr_p);
  *pnread = 0;

  asynPrint(pasynUser, ASYN_TRACE_INFO,
            "%swriteWriteReadAds outlen=%u ams_payload_len=0x%x ads_len=0x%x\n",
            modNamEMC,(unsigned)outlen,
            ams_payload_len, ads_len);
  ams_hdr_p->ams_tcp_hdr.length_0 = (uint8_t)ams_payload_len;
  ams_hdr_p->ams_tcp_hdr.length_1 = (uint8_t)(ams_payload_len >> 8);
  ams_hdr_p->ams_tcp_hdr.length_2 = (uint8_t)(ams_payload_len >> 16);
  ams_hdr_p->ams_tcp_hdr.length_3 = (uint8_t)(ams_payload_len >> 24);
  memcpy(&ams_hdr_p->target,
         &ctrlLocal.remote,  sizeof(ams_hdr_p->target));
  memcpy(&ams_hdr_p->source,
         &ctrlLocal.local, sizeof(ams_hdr_p->source));
  ams_hdr_p->cmdID_low  = (uint8_t)ads_cmdID;
  ams_hdr_p->cmdID_high = (uint8_t)(ads_cmdID >> 8);
  ams_hdr_p->stateFlags_low = 0x4; /* Command */
  ams_hdr_p->length_0 = (uint8_t)ads_len;
  ams_hdr_p->length_1 = (uint8_t)(ads_len >> 8);
  ams_hdr_p->length_2 = (uint8_t)(ads_len >> 16);
  ams_hdr_p->length_3 = (uint8_t)(ads_len >> 24);

  ams_hdr_p->invokeID_0 = (uint8_t)invokeID;
  ams_hdr_p->invokeID_1 = (uint8_t)(invokeID >> 8);
  ams_hdr_p->invokeID_2 = (uint8_t)(invokeID >> 16);
  ams_hdr_p->invokeID_3 = (uint8_t)(invokeID >> 24);

  return asynSuccess;
  return writeReadBinaryOnErrorDisconnect_C(pasynUser,
                                            (const char *)ams_hdr_p, outlen,
                                            (char *)indata, inlen,
                                            &nwrite, pnread,
                                            &eomReason);


}
asynStatus EthercatMCController::getPlcMemoryViaADS(unsigned indexGroup,
                                                    unsigned indexOffset,
                                                    void *data,
                                                    size_t lenInPlc)
{
  int tracelevel = ASYN_TRACE_INFO;
  asynUser *pasynUser = pasynUserController_;
  ads_read_req_type ads_read_req;

  size_t read_buf_len = sizeof(ADS_Read_rep_type) + lenInPlc;
  void *p_read_buf = malloc(read_buf_len);

  asynStatus status;
  uint32_t ams_payload_len = sizeof(ads_read_req.ams_hdr) -
                             sizeof(ads_read_req.ams_hdr.ams_tcp_hdr) + 12;
  size_t nwrite = 0;
  size_t nread = 0;
  int eomReason = 0;

  memset(&ads_read_req, 0, sizeof(ads_read_req));
  memset(p_read_buf, 0, read_buf_len);
  invokeID++;

  ads_read_req.ams_hdr.ams_tcp_hdr.length_0 = (uint8_t)ams_payload_len;
  ads_read_req.ams_hdr.ams_tcp_hdr.length_1 = (uint8_t)(ams_payload_len >> 8);
  ads_read_req.ams_hdr.ams_tcp_hdr.length_2 = (uint8_t)(ams_payload_len >> 16);
  ads_read_req.ams_hdr.ams_tcp_hdr.length_3 = (uint8_t)(ams_payload_len >> 24);
  memcpy(&ads_read_req.ams_hdr.target,
         &ctrlLocal.remote,  sizeof(ads_read_req.ams_hdr.target));
  memcpy(&ads_read_req.ams_hdr.source,
         &ctrlLocal.local, sizeof(ads_read_req.ams_hdr.source));
  ads_read_req.ams_hdr.cmdID_low = ADS_READ;
  ads_read_req.ams_hdr.stateFlags_low = 0x4; /* Command */
  ads_read_req.ams_hdr.length_0 = 12;
  //ads_read_req.ams_hdr.length_1
  //ads_read_req.ams_hdr.length_2
  //ads_read_req.ams_hdr.length_3

  ads_read_req.ams_hdr.invokeID_0 = (uint8_t)invokeID;
  ads_read_req.ams_hdr.invokeID_1 = (uint8_t)(invokeID >> 8);
  ads_read_req.ams_hdr.invokeID_2 = (uint8_t)(invokeID >> 16);
  ads_read_req.ams_hdr.invokeID_3 = (uint8_t)(invokeID >> 24);
  ads_read_req.indexGroup_0 = (uint8_t)indexGroup;
  ads_read_req.indexGroup_1 = (uint8_t)(indexGroup >> 8);
  ads_read_req.indexGroup_2 = (uint8_t)(indexGroup >> 16);
  ads_read_req.indexGroup_3 = (uint8_t)(indexGroup >> 24);
  ads_read_req.indexOffset_0 = (uint8_t)indexOffset;
  ads_read_req.indexOffset_1 = (uint8_t)(indexOffset >> 8);
  ads_read_req.indexOffset_2 = (uint8_t)(indexOffset >> 16);
  ads_read_req.indexOffset_3 = (uint8_t)(indexOffset >> 24);
  ads_read_req.length_0 = (uint8_t)lenInPlc;
  ads_read_req.length_1 = (uint8_t)(lenInPlc >> 8);
  ads_read_req.length_2 = (uint8_t)(lenInPlc >> 16);
  ads_read_req.length_3 = (uint8_t)(lenInPlc >> 24);

#if 0
  EthercatMCamsdump(pasynUser, tracelevel, "RDMEM", &ads_read_req.ams_hdr);
  EthercatMChexdump(pasynUser, tracelevel, "RDMO",
                    &ads_read_req, sizeof(ads_read_req));
#endif
#if 0
  (void)writeWriteReadAds(pasynUser,
                          (ams_hdr_type *)&ads_read_req, sizeof(ads_read_req),
                          invokeID, ADS_READ,
                          (char*)p_read_buf, read_buf_len,
                          &nread);
#endif
  status = writeReadBinaryOnErrorDisconnect_C(pasynUser,
                                              (const char*)&ads_read_req, sizeof(ads_read_req),
                                              (char*)p_read_buf, read_buf_len,
                                              &nwrite, &nread, &eomReason);

  asynPrint(pasynUser, ASYN_TRACE_INFO,
            "%sYYYYpasynOctetSyncIO->writeRead nread=%u, size_t(ams_rep)=%u eomReason=0x%x lenInPlc=%u status=%d\n",
            modNamEMC,  (unsigned)nread,
            (unsigned) read_buf_len,
            eomReason,
            (unsigned)lenInPlc, (int)status);

  if (!status)
  {
    ADS_Read_rep_type *ADS_Read_rep_p = (ADS_Read_rep_type*) p_read_buf;
    ams_hdr_type *ams_hdr_p = (ams_hdr_type*)ADS_Read_rep_p;
    uint16_t cmdId = ams_hdr_p->cmdID_low + (ams_hdr_p->cmdID_high << 8);
    uint32_t ams_tcp_hdr_len = ams_hdr_p->ams_tcp_hdr.length_0 +
      (ams_hdr_p->ams_tcp_hdr.length_1 << 8) +
      (ams_hdr_p->ams_tcp_hdr.length_2 << 16) +
      (ams_hdr_p->ams_tcp_hdr.length_3 <<24);
    uint32_t ams_lenght = ams_hdr_p->length_0 +
      (ams_hdr_p->length_1 << 8) +
      (ams_hdr_p->length_2 << 16) +
      (ams_hdr_p->length_3 << 24);
    uint32_t ams_errorCode = ams_hdr_p->errorCode_0 +
      (ams_hdr_p->errorCode_1 << 8) +
      (ams_hdr_p->errorCode_2 << 16) +
      (ams_hdr_p->errorCode_3 << 24);
    uint32_t ams_invokeID = ams_hdr_p->invokeID_0 +
      (ams_hdr_p->invokeID_1 << 8) +
      (ams_hdr_p->invokeID_2 << 16) +
      (ams_hdr_p->invokeID_3 << 24);

    uint32_t ads_result = ADS_Read_rep_p->response.result_0 +
      (ADS_Read_rep_p->response.result_1 << 8) +
      (ADS_Read_rep_p->response.result_2 << 16) +
      (ADS_Read_rep_p->response.result_3 << 24);
    uint32_t ads_length = ADS_Read_rep_p->response.length_0 +
      (ADS_Read_rep_p->response.length_1 << 8) +
      (ADS_Read_rep_p->response.length_2 << 16) +
      (ADS_Read_rep_p->response.length_3 << 24);


    //EthercatMChexdump(pasynUser, tracelevel, "IN", p_read_buf, nread);

    asynPrint(pasynUser, tracelevel,
              "nread=%lu ams_tcp_hdr_len=%u ams target=%d.%d.%d.%d.%d.%d:%d source=%d.%d.%d.%d.%d.%d:%d\n",
              (unsigned long)nread, ams_tcp_hdr_len,
              ams_hdr_p->target.netID[0], ams_hdr_p->target.netID[1],
              ams_hdr_p->target.netID[2], ams_hdr_p->target.netID[3],
              ams_hdr_p->target.netID[4], ams_hdr_p->target.netID[5],
              ams_hdr_p->target.port_low + (ams_hdr_p->target.port_high << 8),
              ams_hdr_p->source.netID[0],  ams_hdr_p->source.netID[1],
              ams_hdr_p->source.netID[2],  ams_hdr_p->source.netID[3],
              ams_hdr_p->source.netID[4],  ams_hdr_p->source.netID[5],
              ams_hdr_p->source.port_low + (ams_hdr_p->source.port_high << 8)
              );
    asynPrint(pasynUser, tracelevel,
              "ams_hdr cmd=%u flags=%u ams_len=%u ams_err=%u id=%u ads_result=0x%x ads_length=%u\n",
              cmdId,
              ams_hdr_p->stateFlags_low + (ams_hdr_p->stateFlags_high << 8),
              ams_lenght, ams_errorCode, ams_invokeID,
              ads_result, ads_length);

    asynPrint(pasynUser, tracelevel,
              "%s RDMEM indexGroup=0x%x indexOffset=%u lenInPlc=%u\n",
              modNamEMC, indexGroup, indexOffset, (unsigned)lenInPlc);
    if (!status && !ads_result) {
      uint8_t *src_ptr = (uint8_t*) p_read_buf;
      src_ptr += sizeof(ADS_Read_rep_type);
      memcpy(data, src_ptr, ads_length);
      EthercatMChexdump(pasynUser, tracelevel, "RDMEM",
                        src_ptr, ads_length);
    }
  }
  free(p_read_buf);
  return status;
}

asynStatus EthercatMCController::setPlcMemoryViaADS(unsigned indexGroup,
                                                    unsigned indexOffset,
                                                    const void *data,
                                                    size_t lenInPlc)
{
  int tracelevel = ASYN_TRACE_INFO;
  asynUser *pasynUser = pasynUserController_;
  ADS_Write_rep_type ADS_Write_rep;
  ADS_Write_req_type *ads_write_req_p;

  size_t write_buf_len = sizeof(ADS_Write_req_type) + lenInPlc;
  void *p_write_buf = malloc(write_buf_len);

  asynStatus status;
  uint32_t ams_payload_len = sizeof(ads_write_req_p->ams_hdr) -
                             sizeof(ads_write_req_p->ams_hdr.ams_tcp_hdr) +
                             lenInPlc;
  size_t nwrite = 0;
  size_t nread = 0;
  int eomReason = 0;
  ads_write_req_p = (ADS_Write_req_type *)p_write_buf;

  memset(p_write_buf, 0, write_buf_len);
  memset(&ADS_Write_rep, 0, sizeof(ADS_Write_rep));
  invokeID++;

  ads_write_req_p->ams_hdr.ams_tcp_hdr.length_0 = (uint8_t)ams_payload_len;
  ads_write_req_p->ams_hdr.ams_tcp_hdr.length_1 = (uint8_t)(ams_payload_len >> 8);
  ads_write_req_p->ams_hdr.ams_tcp_hdr.length_2 = (uint8_t)(ams_payload_len >> 16);
  ads_write_req_p->ams_hdr.ams_tcp_hdr.length_3 = (uint8_t)(ams_payload_len >> 24);
  memcpy(&ads_write_req_p->ams_hdr.target,
         &ctrlLocal.remote,  sizeof(ads_write_req_p->ams_hdr.target));
  memcpy(&ads_write_req_p->ams_hdr.source,
         &ctrlLocal.local, sizeof(ads_write_req_p->ams_hdr.source));
  ads_write_req_p->ams_hdr.cmdID_low = ADS_WRITE;
  ads_write_req_p->ams_hdr.stateFlags_low = 0x4; /* Command */
  {
    uint32_t ams_hdr_length = 12 + lenInPlc;
    ads_write_req_p->ams_hdr.length_0 = ams_hdr_length;
    ads_write_req_p->ams_hdr.length_1 = (ams_hdr_length>> 8);
    ads_write_req_p->ams_hdr.length_2 = (ams_hdr_length>> 16);
    ads_write_req_p->ams_hdr.length_3 = (ams_hdr_length>> 24);
  }
  ads_write_req_p->ams_hdr.invokeID_0 = (uint8_t)invokeID;
  ads_write_req_p->ams_hdr.invokeID_1 = (uint8_t)(invokeID >> 8);
  ads_write_req_p->ams_hdr.invokeID_2 = (uint8_t)(invokeID >> 16);
  ads_write_req_p->ams_hdr.invokeID_3 = (uint8_t)(invokeID >> 24);
  ads_write_req_p->indexGroup_0 = (uint8_t)indexGroup;
  ads_write_req_p->indexGroup_1 = (uint8_t)(indexGroup >> 8);
  ads_write_req_p->indexGroup_2 = (uint8_t)(indexGroup >> 16);
  ads_write_req_p->indexGroup_3 = (uint8_t)(indexGroup >> 24);
  ads_write_req_p->indexOffset_0 = (uint8_t)indexOffset;
  ads_write_req_p->indexOffset_1 = (uint8_t)(indexOffset >> 8);
  ads_write_req_p->indexOffset_2 = (uint8_t)(indexOffset >> 16);
  ads_write_req_p->indexOffset_3 = (uint8_t)(indexOffset >> 24);
  ads_write_req_p->length_0 = (uint8_t)lenInPlc;
  ads_write_req_p->length_1 = (uint8_t)(lenInPlc >> 8);
  ads_write_req_p->length_2 = (uint8_t)(lenInPlc >> 16);
  ads_write_req_p->length_3 = (uint8_t)(lenInPlc >> 24);

  asynPrint(pasynUser, tracelevel,
            "%s WR indexGroup=0x%x indexOffset=%u lenInPlc=%u\n",
            modNamEMC, indexGroup, indexOffset, (unsigned)lenInPlc
            );
  (void)writeWriteReadAds(pasynUser,
                          (ams_hdr_type *)p_write_buf, write_buf_len,
                          invokeID, ADS_WRITE,
                          &ADS_Write_rep, sizeof(ADS_Write_rep),
                          &nread);

  /* copy the payload */
  {
    uint8_t *dst_ptr = (uint8_t*)p_write_buf;
    dst_ptr += sizeof(ADS_Write_req_type);
    memcpy(dst_ptr, data, lenInPlc);
    EthercatMChexdump(pasynUser, tracelevel, "WR",
                      data, lenInPlc);
  }

  EthercatMCamsdump(pasynUser, tracelevel, "WRMO", &ads_write_req_p->ams_hdr);

  status = writeReadBinaryOnErrorDisconnect_C(pasynUser,
                                              (const char*)p_write_buf, write_buf_len,
                                              (char*)&ADS_Write_rep, sizeof(ADS_Write_rep),
                                              &nwrite, &nread, &eomReason);

  {
    ams_hdr_type *ams_hdr_p = (ams_hdr_type*)&ADS_Write_rep;
    uint16_t cmdId = ams_hdr_p->cmdID_low + (ams_hdr_p->cmdID_high << 8);
    uint32_t ams_tcp_hdr_len = ams_hdr_p->ams_tcp_hdr.length_0 +
      (ams_hdr_p->ams_tcp_hdr.length_1 << 8) +
      (ams_hdr_p->ams_tcp_hdr.length_2 << 16) +
      (ams_hdr_p->ams_tcp_hdr.length_3 <<24);
    uint32_t ams_lenght = ams_hdr_p->length_0 +
      (ams_hdr_p->length_1 << 8) +
      (ams_hdr_p->length_2 << 16) +
      (ams_hdr_p->length_3 << 24);
    uint32_t ams_errorCode = ams_hdr_p->errorCode_0 +
      (ams_hdr_p->errorCode_1 << 8) +
      (ams_hdr_p->errorCode_2 << 16) +
      (ams_hdr_p->errorCode_3 << 24);
    uint32_t ams_invokeID = ams_hdr_p->invokeID_0 +
      (ams_hdr_p->invokeID_1 << 8) +
      (ams_hdr_p->invokeID_2 << 16) +
      (ams_hdr_p->invokeID_3 << 24);
    uint32_t ads_result = ADS_Write_rep.response.result_0 +
      (ADS_Write_rep.response.result_1 << 8) +
      (ADS_Write_rep.response.result_2 << 16) +
      (ADS_Write_rep.response.result_3 << 24);

    asynPrint(pasynUser, tracelevel,
              "nwrite=%lu ams_tcp_hdr_len=%u ams target=%d.%d.%d.%d.%d.%d:%d source=%d.%d.%d.%d.%d.%d:%d\n",
              (unsigned long)nwrite, ams_tcp_hdr_len,
              ams_hdr_p->target.netID[0], ams_hdr_p->target.netID[1],
              ams_hdr_p->target.netID[2], ams_hdr_p->target.netID[3],
              ams_hdr_p->target.netID[4], ams_hdr_p->target.netID[5],
              ams_hdr_p->target.port_low + (ams_hdr_p->target.port_high << 8),
              ams_hdr_p->source.netID[0], ams_hdr_p->source.netID[1],
              ams_hdr_p->source.netID[2], ams_hdr_p->source.netID[3],
              ams_hdr_p->source.netID[4], ams_hdr_p->source.netID[5],
              ams_hdr_p->source.port_low + (ams_hdr_p->source.port_high << 8)
              );
    asynPrint(pasynUser, tracelevel,
              "ams_hdr cmd=%u flags=%u ams_len=%u ams_err=%u id=%u ads_result=0x%x\n",
              cmdId,
              ams_hdr_p->stateFlags_low + (ams_hdr_p->stateFlags_high << 8),
              ams_lenght, ams_errorCode, ams_invokeID, ads_result);
  }

  asynPrint(pasynUser, ASYN_TRACE_INFO,
            "%sZZZpasynOctetSyncIO->writeRead nread=%u, size_t(ams_rep)=%u eomReason=0x%x\n",
            modNamEMC,  (unsigned)nread,
            (unsigned) sizeof(ADS_Write_rep),
            eomReason);

  return status;
}
