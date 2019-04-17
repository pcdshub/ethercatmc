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
         &ctrlLocal.remote,
         sizeof(ads_read_req.ams_hdr.target));

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
    int len = (int)nread;
    uint8_t *data = (uint8_t *)p_read_buf;
    unsigned pos = 0;
    int tracelevel = ASYN_TRACE_INFO;
    while (len > 0) {
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
