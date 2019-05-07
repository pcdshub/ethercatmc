#include <stdlib.h>
#include <string.h>
#include "EthercatMCController.h"
#include "EthercatMCADSdefs.h"
#include <asynOctetSyncIO.h>

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

#ifndef ASYN_TRACE_DEBUG
#define ASYN_TRACE_DEBUG     0x0080
#endif

//#define DEFAULT_CONTROLLER_TIMEOUT 2.0

static uint32_t invokeID;
static int deftracelevel = ASYN_TRACE_DEBUG;

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
  const AmsHdrType *amsHdr_p = (const AmsHdrType *)(ams_headdr_p);\
  uint32_t amsTcpHdr_len = amsHdr_p->amsTcpHdr.net_len[0] +\
    (amsHdr_p->amsTcpHdr.net_len[1] << 8) +\
    (amsHdr_p->amsTcpHdr.net_len[2] << 16) +\
    (amsHdr_p->amsTcpHdr.net_len[3] <<24);\
    uint32_t ams_lenght = amsHdr_p->net_len[0] +\
      (amsHdr_p->net_len[1] << 8) +\
      (amsHdr_p->net_len[2] << 16) +\
      (amsHdr_p->net_len[3] << 24);\
    uint32_t ams_errorCode = amsHdr_p->net_errCode[0] +\
      (amsHdr_p->net_errCode[1] << 8) +\
      (amsHdr_p->net_errCode[2] << 16) +\
      (amsHdr_p->net_errCode[3] << 24);\
    uint32_t ams_invokeID = amsHdr_p->net_invokeID[0] +\
      (amsHdr_p->net_invokeID[1] << 8) +\
      (amsHdr_p->net_invokeID[2] << 16) +\
      (amsHdr_p->net_invokeID[3] << 24);\
  asynPrint(pasynUser, tracelevel,\
            "amsTcpHdr_len=%u ams target=%d.%d.%d.%d.%d.%d:%d "  \
            "source=%d.%d.%d.%d.%d.%d:%d\n",                       \
            amsTcpHdr_len,                                       \
            amsHdr_p->target.netID[0], amsHdr_p->target.netID[1],\
            amsHdr_p->target.netID[2], amsHdr_p->target.netID[3],\
            amsHdr_p->target.netID[4], amsHdr_p->target.netID[5],\
            amsHdr_p->target.port_low + (amsHdr_p->target.port_high << 8),\
            amsHdr_p->source.netID[0],  amsHdr_p->source.netID[1],\
            amsHdr_p->source.netID[2],  amsHdr_p->source.netID[3],\
            amsHdr_p->source.netID[4],  amsHdr_p->source.netID[5],\
            amsHdr_p->source.port_low + (amsHdr_p->source.port_high << 8)\
            );\
  asynPrint(pasynUser, tracelevel,\
            "amsHdr cmd=%u flags=%u ams_len=%u ams_err=%u id=%u\n",\
            amsHdr_p->cmdID_low + (amsHdr_p->cmdID_high <<8),\
            amsHdr_p->stateFlags_low + (amsHdr_p->stateFlags_high << 8),\
            ams_lenght, ams_errorCode, ams_invokeID);\
}\

/****************************************************************************/
extern "C" unsigned netToUint(void *data, size_t lenInPlc)
{
  const uint8_t *src = (const uint8_t*)data;
  unsigned uRes;
  if (lenInPlc == 2) {
    uRes = (unsigned)src[0] + ((unsigned)src[1] << 8);
    return uRes;
  } else if ((lenInPlc == 4) || (lenInPlc == 8)) {
    /* We don't use the full range of 64 bit integers,
       only values up to 2^31 */
    uRes = (unsigned)src[0] + ((unsigned)src[1] << 8) +
           ((unsigned)src[2] << 16) + ((unsigned)src[3] << 24);
    return uRes;
  }
  return 0;
}

extern "C" double netToDouble(void *data, size_t lenInPlc)
{
  const uint8_t *src = (const uint8_t*)data;
  if (lenInPlc == 4) {
    union {
      volatile uint32_t uRes;
      volatile float    fRes;
    } dst;
    dst.uRes = (uint32_t)src[0] + ((uint32_t)src[1] << 8) +
               ((uint32_t)src[2] << 16) + ((uint32_t)src[3] << 24);
    return (double)dst.fRes;
  } else if (lenInPlc == 8) {
    union {
      volatile uint64_t uRes;
      volatile double   fRes;
    } dst;
    dst.uRes = (uint64_t)src[0] + ((uint64_t)src[1] << 8) +
               ((uint64_t)src[2] << 16) + ((uint64_t)src[3] << 24) +
               ((uint64_t)src[4] << 32) + ((uint64_t)src[5] << 40) +
               ((uint64_t)src[6] << 48) + ((uint64_t)src[7] << 56);
    return dst.fRes;
  } else {
    return 0.0;
  }
}

extern "C" void doubleToNet(const double value, void *data, size_t lenInPlc)
{
  uint8_t *dst = (uint8_t*)data;
  if (lenInPlc == 4) {
    union {
      volatile uint32_t uRes;
      volatile float    fRes;
    } src;
    src.fRes = (float)value;
    dst[0] = (uint8_t)src.uRes;
    dst[1] = (uint8_t)(src.uRes >> 8);
    dst[2] = (uint8_t)(src.uRes >> 16);
    dst[3] = (uint8_t)(src.uRes >> 24);
  } else if (lenInPlc == 8) {
    union {
      volatile uint64_t uRes;
      volatile double   fRes;
    } src;
    src.fRes = value;
    dst[0] = (uint8_t)src.uRes;
    dst[1] = (uint8_t)(src.uRes >> 8);
    dst[2] = (uint8_t)(src.uRes >> 16);
    dst[3] = (uint8_t)(src.uRes >> 24);
    dst[4] = (uint8_t)(src.uRes >> 32);
    dst[5] = (uint8_t)(src.uRes >> 40);
    dst[6] = (uint8_t)(src.uRes >> 48);
    dst[7] = (uint8_t)(src.uRes >> 56);
  } else {
    memset(data, 0, lenInPlc);
  }
}

extern "C" void uintToNet(const unsigned value, void *data, size_t lenInPlc)
{
  uint8_t *dst = (uint8_t*)data;
  memset(data, 0, lenInPlc);
  if (lenInPlc == 2) {
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
  } else if ((lenInPlc == 4) || (lenInPlc == 8)) {
    /* We don't use the full range of 64 bit integers,
       only values up to 2^31 */
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
    dst[2] = (uint8_t)(value >> 16);
    dst[3] = (uint8_t)(value >> 24);
  }
}



extern "C"
asynStatus writeReadBinaryOnErrorDisconnect_C(asynUser *pasynUser,
                                              const char *outdata,
                                              size_t outlen,
                                              char *indata, size_t inlen,
                                              size_t *pnwrite, size_t *pnread,
                                              int *peomReason)
{
  int tracelevel = deftracelevel;
  int errorProblem = 0;
  char old_InputEos[10];
  int old_InputEosLen = 0;
  char old_OutputEos[10];
  int old_OutputEosLen = 0;
  int eomReason;
  size_t nread;
  uint32_t part_1_len = sizeof(AmsTcpHdrType);
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
  status = pasynOctetSyncIO->write(pasynUser, outdata, outlen,
                                   DEFAULT_CONTROLLER_TIMEOUT,
                                   pnwrite);
  if (*pnwrite != outlen) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s outlen=%lu nwrite=%lu timeout=%f status=%d\n",
              modNamEMC,
              (unsigned long)outlen,
              (unsigned long)*pnwrite,
              DEFAULT_CONTROLLER_TIMEOUT,
              status);
    status = asynError; /* TimeOut -> Error */
    return status;
  }
  EthercatMChexdump(pasynUser, tracelevel, "OUT",
                    outdata, outlen);
  /* Read the AMS/TCP Header */
  status = pasynOctetSyncIO->read(pasynUser,
                                  indata, part_1_len,
                                  DEFAULT_CONTROLLER_TIMEOUT,
                                  &nread, &eomReason);
  EthercatMChexdump(pasynUser, tracelevel, "IN ams/tcp ",
                    indata, nread);
  if (nread != part_1_len) {
    if (nread) {
      EthercatMCamsdump(pasynUser, tracelevel, "IN ", indata);
      EthercatMChexdump(pasynUser, tracelevel, "IN ",
                        indata, nread);
    }
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s calling disconnect_C nread=%lu timeout=%f eomReason=%x (%s%s%s) status=%d\n",
              modNamEMC,
              (unsigned long)*pnread,
              DEFAULT_CONTROLLER_TIMEOUT,
              eomReason,
              eomReason & ASYN_EOM_CNT ? "CNT" : "",
              eomReason & ASYN_EOM_EOS ? "EOS" : "",
              eomReason & ASYN_EOM_END ? "END" : "",
              status);
    disconnect_C(pasynUser);
    *peomReason = eomReason;
    status = asynError;
  }
  if (!status) {
    /* The length to read is inside the AMS/TCP header */
    const AmsHdrType *amsHdr_p = (const AmsHdrType *)indata;
    uint32_t amsTcpHdr_len = amsHdr_p->amsTcpHdr.net_len[0] +
      (amsHdr_p->amsTcpHdr.net_len[1] << 8) +
      (amsHdr_p->amsTcpHdr.net_len[2] << 16) +
      (amsHdr_p->amsTcpHdr.net_len[3] <<24);

    uint32_t toread = amsTcpHdr_len; // XX careful when changing things here

    /* Read the rest into indata */
    status = pasynOctetSyncIO->read(pasynUser,
                                    indata + part_1_len,
                                    toread,
                                    DEFAULT_CONTROLLER_TIMEOUT,
                                    &nread, &eomReason);

    if ((status == asynTimeout) ||
        (!status && !nread && (*peomReason & ASYN_EOM_END))) {
      errorProblem = 1;
      tracelevel |= ASYN_TRACE_ERROR;
    }
    asynPrint(pasynUser, tracelevel,
              "%s IN part 2 inlen-part_1_len=%lu toread=0x%x %u nread=%lu status=%d\n",
              modNamEMC,
              (unsigned long)inlen - (unsigned long)part_1_len,
              toread, toread,
              (unsigned long)nread,
              status);
    EthercatMChexdump(pasynUser, tracelevel, "IN part 2",
                      indata, nread);
    if (errorProblem) {
      EthercatMCamsdump(pasynUser, tracelevel, "OUT", outdata);
      EthercatMChexdump(pasynUser, tracelevel, "OUT",
                        outdata, outlen);
      if (nread) {
        EthercatMCamsdump(pasynUser, tracelevel, "IN ", indata);
        EthercatMChexdump(pasynUser, tracelevel, "IN ",
                          indata, nread + part_1_len);
      } else {
        asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%s calling disconnect_C nread=%lu timeout=%f eomReason=%x (%s%s%s) status=%d\n",
                  modNamEMC,
                  (unsigned long)*pnread,
                  DEFAULT_CONTROLLER_TIMEOUT,
                  eomReason,
                  eomReason & ASYN_EOM_CNT ? "CNT" : "",
                  eomReason & ASYN_EOM_EOS ? "EOS" : "",
                  eomReason & ASYN_EOM_END ? "END" : "",
                  status);
        disconnect_C(pasynUser);
        status = asynError; /* TimeOut -> Error */
      }
    } else {
      *pnread = nread + part_1_len;
      *peomReason = eomReason;
    }
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
                                                   AmsHdrType *ams_req_hdr_p,
                                                   size_t outlen,
                                                   uint32_t invokeID,
                                                   uint32_t ads_cmdID,
                                                   void *indata, size_t inlen,
                                                   size_t *pnread)
{
  size_t nwrite = 0;
  int eomReason = 0;
  asynStatus status;
  uint32_t ams_payload_len = outlen - sizeof(ams_req_hdr_p->amsTcpHdr);
  uint32_t ads_len = outlen - sizeof(*ams_req_hdr_p);
  *pnread = 0;

  ams_req_hdr_p->amsTcpHdr.net_len[0] = (uint8_t)ams_payload_len;
  ams_req_hdr_p->amsTcpHdr.net_len[1] = (uint8_t)(ams_payload_len >> 8);
  ams_req_hdr_p->amsTcpHdr.net_len[2] = (uint8_t)(ams_payload_len >> 16);
  ams_req_hdr_p->amsTcpHdr.net_len[3] = (uint8_t)(ams_payload_len >> 24);
  memcpy(&ams_req_hdr_p->target,
         &ctrlLocal.remote,  sizeof(ams_req_hdr_p->target));
  memcpy(&ams_req_hdr_p->source,
         &ctrlLocal.local, sizeof(ams_req_hdr_p->source));
  ams_req_hdr_p->cmdID_low  = (uint8_t)ads_cmdID;
  ams_req_hdr_p->cmdID_high = (uint8_t)(ads_cmdID >> 8);
  ams_req_hdr_p->stateFlags_low = 0x4; /* Command */
  ams_req_hdr_p->net_len[0] = (uint8_t)ads_len;
  ams_req_hdr_p->net_len[1] = (uint8_t)(ads_len >> 8);
  ams_req_hdr_p->net_len[2] = (uint8_t)(ads_len >> 16);
  ams_req_hdr_p->net_len[3] = (uint8_t)(ads_len >> 24);

  ams_req_hdr_p->net_invokeID[0] = (uint8_t)invokeID;
  ams_req_hdr_p->net_invokeID[1] = (uint8_t)(invokeID >> 8);
  ams_req_hdr_p->net_invokeID[2] = (uint8_t)(invokeID >> 16);
  ams_req_hdr_p->net_invokeID[3] = (uint8_t)(invokeID >> 24);

  status = writeReadBinaryOnErrorDisconnect_C(pasynUser,
                                              (const char *)ams_req_hdr_p,
                                              outlen,
                                              (char *)indata, inlen,
                                              &nwrite, pnread,
                                              &eomReason);
  if (!status) {
    size_t nread = *pnread;
    AmsHdrType *ams_rep_hdr_p = (AmsHdrType*)indata;
    uint32_t amsTcpHdr_len = ams_rep_hdr_p->amsTcpHdr.net_len[0] +
      (ams_rep_hdr_p->amsTcpHdr.net_len[1] << 8) +
      (ams_rep_hdr_p->amsTcpHdr.net_len[2] << 16) +
      (ams_rep_hdr_p->amsTcpHdr.net_len[3] <<24);
    if (amsTcpHdr_len  != (nread - sizeof(ams_rep_hdr_p->amsTcpHdr))) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%s nread=%u amsTcpHdr_len=%u\n", modNamEMC,
                (unsigned)nread, amsTcpHdr_len);
      status = asynError;
    }
    if (!status) {
      uint32_t ads_rep_len = ams_rep_hdr_p->net_len[0] +
        (ams_rep_hdr_p->net_len[1] << 8) +
        (ams_rep_hdr_p->net_len[2] << 16) +
        (ams_rep_hdr_p->net_len[3] << 24);
      if (ads_rep_len != (nread - sizeof(AmsHdrType))) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%s warning ?? nread=%u ads_rep_len=%u\n", modNamEMC,
                  (unsigned)nread, ads_rep_len);
        //status = asynError;
      }
    }
    if (!status) {
      uint32_t ams_errorCode = ams_rep_hdr_p->net_errCode[0] +
        (ams_rep_hdr_p->net_errCode[1] << 8) +
        (ams_rep_hdr_p->net_errCode[2] << 16) +
        (ams_rep_hdr_p->net_errCode[3] << 24);
      if (ams_errorCode) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%s nread=%u ams_errorCode=0x%x\n", modNamEMC,
                  (unsigned)nread, ams_errorCode);
        status = asynError;
      }
    }
    if (!status) {
      uint32_t rep_invokeID = ams_rep_hdr_p->net_invokeID[0] +
        (ams_rep_hdr_p->net_invokeID[1] << 8) +
        (ams_rep_hdr_p->net_invokeID[2] << 16) +
        (ams_rep_hdr_p->net_invokeID[3] << 24);

      if (invokeID != rep_invokeID) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%s invokeID=0x%x rep_invokeID=0x%x\n", modNamEMC,
                  invokeID, rep_invokeID);
        status = asynError;
      }
    }
  }
  return status;
}

asynStatus EthercatMCController::getPlcMemoryViaADS(unsigned indexOffset,
                                                    void *data,
                                                    size_t lenInPlc)
{
  static unsigned indexGroup = 0x4020;
  int tracelevel = deftracelevel;
  asynUser *pasynUser = pasynUserController_;
  AdsReadReqType ads_read_req;

  size_t read_buf_len = sizeof(AdsReadRepType) + lenInPlc;
  void *p_read_buf = malloc(read_buf_len);

  asynStatus status;
  size_t nread = 0;

  memset(&ads_read_req, 0, sizeof(ads_read_req));
  memset(p_read_buf, 0, read_buf_len);
  invokeID++;

  ads_read_req.net_idxGrp[0] = (uint8_t)indexGroup;
  ads_read_req.net_idxGrp[1] = (uint8_t)(indexGroup >> 8);
  ads_read_req.net_idxGrp[2] = (uint8_t)(indexGroup >> 16);
  ads_read_req.net_idxGrp[3] = (uint8_t)(indexGroup >> 24);
  ads_read_req.net_idxOff[0] = (uint8_t)indexOffset;
  ads_read_req.net_idxOff[1] = (uint8_t)(indexOffset >> 8);
  ads_read_req.net_idxOff[2] = (uint8_t)(indexOffset >> 16);
  ads_read_req.net_idxOff[3] = (uint8_t)(indexOffset >> 24);
  ads_read_req.net_len[0] = (uint8_t)lenInPlc;
  ads_read_req.net_len[1] = (uint8_t)(lenInPlc >> 8);
  ads_read_req.net_len[2] = (uint8_t)(lenInPlc >> 16);
  ads_read_req.net_len[3] = (uint8_t)(lenInPlc >> 24);

  status = writeWriteReadAds(pasynUser,
                             (AmsHdrType *)&ads_read_req,
                             sizeof(ads_read_req),
                             invokeID, ADS_READ,
                             (char*)p_read_buf, read_buf_len,
                             &nread);
  asynPrint(pasynUser, tracelevel,
            "%s RDMEM indexOffset=%u lenInPlc=%u status=%d\n",
            modNamEMC, indexOffset, (unsigned)lenInPlc, (int)status);
  if (!status)
  {
    AdsReadRepType *ADS_Read_rep_p = (AdsReadRepType*) p_read_buf;
    uint32_t ads_result = ADS_Read_rep_p->response.net_res[0] +
      (ADS_Read_rep_p->response.net_res[1] << 8) +
      (ADS_Read_rep_p->response.net_res[2] << 16) +
      (ADS_Read_rep_p->response.net_res[3] << 24);
    uint32_t ads_length = ADS_Read_rep_p->response.net_len[0] +
      (ADS_Read_rep_p->response.net_len[1] << 8) +
      (ADS_Read_rep_p->response.net_len[2] << 16) +
      (ADS_Read_rep_p->response.net_len[3] << 24);

    if (ads_result) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%sads_result=0x%x\n", modNamEMC, ads_result);
      status = asynError;
    }
    if (ads_length != lenInPlc) {
        asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%slenInPlc=%lu ads_length=%u\n", modNamEMC,
                  (unsigned long)lenInPlc,ads_length);
        status = asynError;
    }
    if (!status) {
      uint8_t *src_ptr = (uint8_t*) p_read_buf;
      src_ptr += sizeof(AdsReadRepType);
      memcpy(data, src_ptr, ads_length);
      EthercatMChexdump(pasynUser, tracelevel, "RDMEM",
                        src_ptr, ads_length);
    }
  }
  free(p_read_buf);
  return status;
}

asynStatus EthercatMCController::setPlcMemoryViaADS(unsigned indexOffset,
                                                    const void *data,
                                                    size_t lenInPlc)
{
  static unsigned indexGroup = 0x4020;
  int tracelevel = deftracelevel;
  asynUser *pasynUser = pasynUserController_;
  AdsWriteRepType ADS_Write_rep;
  AdsWriteReqType *ads_write_req_p;

  size_t write_buf_len = sizeof(AdsWriteReqType) + lenInPlc;
  void *p_write_buf = malloc(write_buf_len);

  asynStatus status;
  size_t nread = 0;
  ads_write_req_p = (AdsWriteReqType *)p_write_buf;

  memset(p_write_buf, 0, write_buf_len);
  memset(&ADS_Write_rep, 0, sizeof(ADS_Write_rep));
  invokeID++;

  ads_write_req_p->net_idxGrp[0] = (uint8_t)indexGroup;
  ads_write_req_p->net_idxGrp[1] = (uint8_t)(indexGroup >> 8);
  ads_write_req_p->net_idxGrp[2] = (uint8_t)(indexGroup >> 16);
  ads_write_req_p->net_idxGrp[3] = (uint8_t)(indexGroup >> 24);
  ads_write_req_p->net_idxOff[0] = (uint8_t)indexOffset;
  ads_write_req_p->net_idxOff[1] = (uint8_t)(indexOffset >> 8);
  ads_write_req_p->net_idxOff[2] = (uint8_t)(indexOffset >> 16);
  ads_write_req_p->net_idxOff[3] = (uint8_t)(indexOffset >> 24);
  ads_write_req_p->net_len[0] = (uint8_t)lenInPlc;
  ads_write_req_p->net_len[1] = (uint8_t)(lenInPlc >> 8);
  ads_write_req_p->net_len[2] = (uint8_t)(lenInPlc >> 16);
  ads_write_req_p->net_len[3] = (uint8_t)(lenInPlc >> 24);

  asynPrint(pasynUser, tracelevel,
            "%s WR indexOffset=%u lenInPlc=%u\n",
            modNamEMC, indexOffset, (unsigned)lenInPlc
            );
  /* copy the payload */
  {
    uint8_t *dst_ptr = (uint8_t*)p_write_buf;
    dst_ptr += sizeof(AdsWriteReqType);
    memcpy(dst_ptr, data, lenInPlc);
    EthercatMChexdump(pasynUser, tracelevel, "WRMEM",
                      data, lenInPlc);
  }
  status = writeWriteReadAds(pasynUser,
                             (AmsHdrType *)p_write_buf, write_buf_len,
                             invokeID, ADS_WRITE,
                             &ADS_Write_rep, sizeof(ADS_Write_rep),
                             &nread);

  if (!status) {
    uint32_t ads_result = ADS_Write_rep.response.net_res[0] +
      (ADS_Write_rep.response.net_res[1] << 8) +
      (ADS_Write_rep.response.net_res[2] << 16) +
      (ADS_Write_rep.response.net_res[3] << 24);

    if (ads_result) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%sads_result=0x%x\n", modNamEMC, ads_result);
      status = asynError;
    }
  }
  return status;
}


asynStatus EthercatMCController::getSymbolInfoViaADS(const char *symbolName,
                                                     void *data,
                                                     size_t lenInPlc)
{
  int tracelevel = deftracelevel | ASYN_TRACE_INFO;
  asynUser *pasynUser = pasynUserController_;
  unsigned indexGroup = 0xF009;
  unsigned indexOffset = 0;
  size_t   symbolNameLen = strlen(symbolName);

  size_t write_buf_len = sizeof(AdsReadWriteReqType) + symbolNameLen;
  size_t read_buf_len  = sizeof(AdsReadWriteRepType) + lenInPlc;
  void *p_read_buf = malloc(read_buf_len);
  AdsReadWriteReqType *ads_read_write_req_p =
    (AdsReadWriteReqType*)malloc(write_buf_len);

  asynStatus status;
  size_t nread = 0;

  memset(ads_read_write_req_p, 0, write_buf_len);
  memset(p_read_buf, 0, read_buf_len);
  invokeID++;

  ads_read_write_req_p->net_idxGrp[0] = (uint8_t)indexGroup;
  ads_read_write_req_p->net_idxGrp[1] = (uint8_t)(indexGroup >> 8);
  ads_read_write_req_p->net_idxGrp[2] = (uint8_t)(indexGroup >> 16);
  ads_read_write_req_p->net_idxGrp[3] = (uint8_t)(indexGroup >> 24);
  ads_read_write_req_p->net_idxOff[0] = (uint8_t)indexOffset;
  ads_read_write_req_p->net_idxOff[1] = (uint8_t)(indexOffset >> 8);
  ads_read_write_req_p->net_idxOff[2] = (uint8_t)(indexOffset >> 16);
  ads_read_write_req_p->net_idxOff[3] = (uint8_t)(indexOffset >> 24);
  ads_read_write_req_p->net_rd_len[0] = (uint8_t)lenInPlc;
  ads_read_write_req_p->net_rd_len[1] = (uint8_t)(lenInPlc >> 8);
  ads_read_write_req_p->net_rd_len[2] = (uint8_t)(lenInPlc >> 16);
  ads_read_write_req_p->net_rd_len[3] = (uint8_t)(lenInPlc >> 24);
  ads_read_write_req_p->net_wr_len[0] = (uint8_t)symbolNameLen;
  ads_read_write_req_p->net_wr_len[1] = (uint8_t)(symbolNameLen >> 8);
  ads_read_write_req_p->net_wr_len[2] = (uint8_t)(symbolNameLen >> 16);
  ads_read_write_req_p->net_wr_len[3] = (uint8_t)(symbolNameLen >> 24);

  /* copy the symbol name */
  {
    uint8_t *dst_ptr = (uint8_t*)ads_read_write_req_p;
    dst_ptr += sizeof(AdsReadWriteReqType);
    memcpy(dst_ptr, symbolName, symbolNameLen);
    EthercatMChexdump(pasynUser, tracelevel, "LOOKS",
                      symbolName, symbolNameLen);
  }
  status = writeWriteReadAds(pasynUser,
                             (AmsHdrType *)ads_read_write_req_p,
                             write_buf_len,
                             invokeID, ADS_READ_WRITE,
                             (char*)p_read_buf, read_buf_len,
                             &nread);
  if (!status)
  {
    AdsReadWriteRepType *adsReadWriteRep_p = (AdsReadWriteRepType*) p_read_buf;
    uint32_t ads_result = adsReadWriteRep_p->response.net_res[0] +
      (adsReadWriteRep_p->response.net_res[1] << 8) +
      (adsReadWriteRep_p->response.net_res[2] << 16) +
      (adsReadWriteRep_p->response.net_res[3] << 24);
    uint32_t ads_length = adsReadWriteRep_p->response.net_len[0] +
      (adsReadWriteRep_p->response.net_len[1] << 8) +
      (adsReadWriteRep_p->response.net_len[2] << 16) +
      (adsReadWriteRep_p->response.net_len[3] << 24);

    if (ads_result) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%sERROR ads_result=0x%x\n", modNamEMC, ads_result);
      status = asynError;
    }
    if (!status) {
      AdsGetSymbolInfoByNameRepType *adsGgetSymbolInfoByName_rep_p;
      adsGgetSymbolInfoByName_rep_p = (AdsGetSymbolInfoByNameRepType *)p_read_buf;
      (void)adsGgetSymbolInfoByName_rep_p;
    }
    if (!status) {
      uint8_t *src_ptr = (uint8_t*) p_read_buf;
      src_ptr += sizeof(AdsReadWriteRepType);
      asynPrint(pasynUser, tracelevel,
                "%s ads_result=0x%x ads_length=0x%x\n",
                modNamEMC, ads_result, ads_length);
      EthercatMChexdump(pasynUser, tracelevel, "IN ADS",
                        src_ptr, ads_length);
      memcpy(data, src_ptr, ads_length);
    }
  }
  free(ads_read_write_req_p);
  free(p_read_buf);
  return status;
}
