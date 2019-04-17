#include <stdlib.h>
#include "EthercatMCController.h"
#include "EthercatMCADSdefs.h"
#include <asynOctetSyncIO.h>

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

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
#if 0
  status = writeReadOnErrorDisconnect_C(pasynUser, outdata, outlen,
                                        indata, inlen);
#else
  status = pasynOctetSyncIO->writeRead(pasynUser, outdata, outlen,
                                       indata, inlen,
                                       DEFAULT_CONTROLLER_TIMEOUT,
                                       pnwrite, pnread, peomReason);
  asynPrint(pasynUser, ASYN_TRACE_INFO,
            "%sXXXXXXXXXXXpasynOctetSyncIO->writeRead outlen=%u inlen=%u nread=%lu, status=%s (%d)\n",
            modNamEMC,  (unsigned)outlen,
            (unsigned)inlen,
            (unsigned long)*pnread,
            pasynManager->strStatus(status), status);

#endif

restore_Eos:
  {
    asynStatus cmdStatus;
    cmdStatus = pasynOctetSyncIO->setInputEos(pasynUser,
                                              old_InputEos, old_InputEosLen);
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%scmdStatus=%s (%d)\n", modNamEMC,
              pasynManager->strStatus(cmdStatus), (int)cmdStatus);
    cmdStatus = pasynOctetSyncIO->setOutputEos(pasynUser,
                                               old_OutputEos,
                                               old_OutputEosLen);
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%scmdStatus=%s (%d)\n", modNamEMC,
              pasynManager->strStatus(cmdStatus), (int)cmdStatus);
  }
  return status;
}


asynStatus EthercatMCController::getPlcMemory(unsigned indexGroup,
                                              unsigned indexOffset,
                                              void *data,
                                              size_t lenInPlc)
{
  asynUser *pasynUser = pasynUserController_;
  ads_read_req_type ads_read_req;

  size_t read_buf_len = sizeof(ADS_Read_rep_type) + lenInPlc;
  void *p_read_buf = malloc(read_buf_len);

  asynStatus status;
  static uint32_t invokeID;
  uint32_t ams_payload_len = sizeof(ads_read_req.ams_hdr) -
                             sizeof(ads_read_req.ams_hdr.ams_tcp_hdr);
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

  status = writeReadBinaryOnErrorDisconnect_C(pasynUser,
                                              (const char*)&ads_read_req, sizeof(ads_read_req),
                                              (char*)p_read_buf, read_buf_len,
                                              &nwrite, &nread, &eomReason);

  {
    int tracelevel = ASYN_TRACE_INFO;
    ADS_Read_rep_type *ADS_Read_rep_p = (ADS_Read_rep_type*) p_read_buf;
    ams_hdr_type *ams_hdr_p = (ams_hdr_type*)ADS_Read_rep_p;
    uint16_t cmdId = ams_hdr_p->cmdID_low + (ams_hdr_p->cmdID_high << 8);
    uint32_t ams_length = ams_hdr_p->ams_tcp_hdr.length_0 +
      (ams_hdr_p->ams_tcp_hdr.length_1 << 8) +
      (ams_hdr_p->ams_tcp_hdr.length_2 << 16) +
      (ams_hdr_p->ams_tcp_hdr.length_3 <<24);

    asynPrint(pasynUser, tracelevel,
              "nread=%lu AMS tcp_hdr.res0=%x tcp_hdr.res1=%x ams_length=%u\n",
              (unsigned long)nread,
              ams_hdr_p->ams_tcp_hdr.res0,
              ams_hdr_p->ams_tcp_hdr.res1,
              ams_length);

    asynPrint(pasynUser, tracelevel,
              "ams_hdr target=%d.%d.%d.%d.%d.%d:%d source=%d.%d.%d.%d.%d.%d:%d\n",
              ams_hdr_p->target.netID[0],
              ams_hdr_p->target.netID[1],
              ams_hdr_p->target.netID[2],
              ams_hdr_p->target.netID[3],
              ams_hdr_p->target.netID[4],
              ams_hdr_p->target.netID[5],
              ams_hdr_p->target.port_low +
              ams_hdr_p->target.port_high * 256,
              ams_hdr_p->source.netID[0],
              ams_hdr_p->source.netID[1],
              ams_hdr_p->source.netID[2],
              ams_hdr_p->source.netID[3],
              ams_hdr_p->source.netID[4],
              ams_hdr_p->source.netID[5],
              ams_hdr_p->source.port_low +
              ams_hdr_p->source.port_high * 256
              );
    asynPrint(pasynUser, tracelevel,
              "ams_hdr cmd=%u flags=%u len=%u err=%u id=%u\n",
              cmdId,
              ams_hdr_p->stateFlags_low +
              (ams_hdr_p->stateFlags_high << 8),
              ams_hdr_p->length_0 +
              (ams_hdr_p->length_1 << 8) +
              (ams_hdr_p->length_2 << 16) +
              (ams_hdr_p->length_3 << 24),
              ams_hdr_p->errorCode_0 +
              (ams_hdr_p->errorCode_1 << 8) +
              (ams_hdr_p->errorCode_2 << 16) +
              (ams_hdr_p->errorCode_3 << 24),
              ams_hdr_p->invokeID_0 +
              (ams_hdr_p->invokeID_1 << 8) +
              (ams_hdr_p->invokeID_2 << 16) +
              (ams_hdr_p->invokeID_3 << 24)
              );
    asynPrint(pasynUser, tracelevel,
              "ads_read_rep result=0x%x len=%u\n",
              ADS_Read_rep_p->response.result_0 +
              (ADS_Read_rep_p->response.result_1 << 8) +
              (ADS_Read_rep_p->response.result_2 << 16) +
              (ADS_Read_rep_p->response.result_3 << 24),
              ADS_Read_rep_p->response.length_0 +
              (ADS_Read_rep_p->response.length_1 << 8) +
              (ADS_Read_rep_p->response.length_2 << 16) +
              (ADS_Read_rep_p->response.length_3 << 24)
              );
  }
  {
    int len = (int)nread;
    uint8_t *data = (uint8_t *)p_read_buf;
    int count;
    unsigned pos = 0;
    int tracelevel = ASYN_TRACE_INFO;
    while (len > 0) {
      struct {
        char asc_txt[8];
        char space[2];
        char hex_txt[8][3];
        char nul;
      } print_buf;
      memset(&print_buf, ' ', sizeof(print_buf));
      print_buf.nul = '\0';
      for (count = 0; count < 8; count++) {
        if (count < len) {
          unsigned char c = (unsigned char)data[count];
          if (c > 0x32 && c < 0x7F)
            print_buf.asc_txt[count] = c;
          else
            print_buf.asc_txt[count] = '.';
          snprintf((char*)&print_buf.hex_txt[count],
                   sizeof(print_buf.hex_txt[count]),
                   "%02x", c);
          /* Replace NUL with ' ' after snprintf */
          print_buf.hex_txt[count][2] = ' ';
        }
      }
#if 1
      asynPrint(pasynUser, tracelevel,
                "%s[%02x]%s\n",
                modNamEMC, pos, (char*)&print_buf);
#else
      asynPrint(pasynUser, tracelevel,
                "%s[%02x]%c%c%c%c%c%c%c%c  %02x %02x %02x %02x %02x %02x %02x %02x\n",
                modNamEMC, pos,
                data[0] >= 0x32 && data[0] < 0x7F ? data[0] : '.',
                data[1] >= 0x32 && data[1] < 0x7F ? data[1] : '.',
                data[2] >= 0x32 && data[2] < 0x7F ? data[2] : '.',
                data[3] >= 0x32 && data[3] < 0x7F ? data[3] : '.',
                data[4] >= 0x32 && data[4] < 0x7F ? data[4] : '.',
                data[5] >= 0x32 && data[5] < 0x7F ? data[5] : '.',
                data[6] >= 0x32 && data[6] < 0x7F ? data[6] : '.',
                data[7] >= 0x32 && data[7] < 0x7F ? data[7] : '.',
                data[0], data[1], data[2], data[3],
                data[4], data[5], data[6], data[7]
                );
#endif
      len -= 8;
      data += 8;
      pos += 8;
    }
  }

  asynPrint(pasynUser, ASYN_TRACE_INFO,
            "%sYYYYpasynOctetSyncIO->writeRead nread=%u, size_t(ams_rep)=%u eomReason=0x%x lenInPlc=%u\n",
            modNamEMC,  (unsigned)nread,
            (unsigned) read_buf_len,
            eomReason,
            (unsigned)lenInPlc);

  if (!status) {
    uint8_t *src_ptr = (uint8_t*) p_read_buf;
    src_ptr += sizeof(ADS_Read_rep_type);
    memcpy(data, src_ptr, lenInPlc);
  }

  free(p_read_buf);
  return status;
}
