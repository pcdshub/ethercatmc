/*
  FILENAME... EthercatMCHelper.cpp
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>

#include "EthercatMCController.h"
#include "EthercatMCIndexerAxis.h"

#include <epicsThread.h>

static unsigned indexGroup = 0x4020;

#define MAX_ADSPORT 853

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

extern "C" {
  const char *plcUnitTxtFromUnitCode(unsigned unitCode)
  {
    const static char *const unitTxts[] = {
      " ",
      "V",
      "A",
      "W",
      "m",
      "gr",
      "Hz",
      "T",
      "K",
      "C",
      "F",
      "bar",
      "degree",
      "Ohm",
      "m/sec",
      "m2/sec",
      "m3/sec",
      "s",
      "counts",
      "bar/sec",
      "bar/sec2",
      "F",
      "H" };
    if (unitCode < sizeof(unitTxts)/sizeof(unitTxts[0]))
      return unitTxts[unitCode];

    return "??";
  }
  const char *plcUnitPrefixTxt(int prefixCode)
  {
    if (prefixCode >= 0) {
      switch (prefixCode) {
        case 24:   return "Y";
        case 21:   return "Z";
        case 18:   return "E";
        case 15:   return "P";
        case 12:   return "T";
        case 9:    return "G";
        case 6:    return "M";
        case 3:    return "k";
        case 2:    return "h";
        case 1:    return "da";
        case 0:    return "";
      default:
        return "?";
      }
    } else {
      switch (-prefixCode) {
        case 1:   return "d";
        case 2:   return "c";
        case 3:   return "m";
        case 6:   return "u";
        case 9:   return "n";
        case 12:  return "p";
        case 15:  return "f";
        case 18:  return "a";
        case 21:  return "z";
        case 24:  return "y";
      default:
        return "?";
      }
    }
  }
};
/*
  1   2   3   4   5   6   7   8   9   10   11   12       13   14   15     16      17  18       19      20       21  22
  V   A   W   m   g   Hz  T   K   °C   °F  bar   °(deg)  Ohm  m/s   m²/s   m³/s   s   counts   bar/s   bar/s²   F   H

  Beispiele für zusammengesetzte Einheiten:

*/

asynStatus EthercatMCController::getPlcMemoryUint(unsigned indexOffset,
                                                  unsigned *value,
                                                  size_t lenInPlc)
{

  int traceMask = 0;
  int nvals;
  int iRes;
  asynStatus status;
  if (ctrlLocal.useADSbinary) {
    uint8_t raw[4];
    unsigned ret;
    if (lenInPlc == 2) {
      status = getPlcMemoryViaADS(indexGroup, indexOffset, raw, lenInPlc);
      ret = (unsigned)raw[0] + (raw[1] << 8);
      *value = ret;
      return status;
    } else if (lenInPlc == 4) {
      status = getPlcMemoryViaADS(indexGroup, indexOffset, raw, sizeof(raw));
      ret = (unsigned)raw[0] + (raw[1] << 8) + (raw[2] << 16) + (raw[3] << 24);
      *value = ret;
      return status;
    } else {
      return asynError;
    }
  }
  if (lenInPlc == 2) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,2,18?",
             ctrlLocal.adsport,
             indexGroup, indexOffset);
  } else if (lenInPlc == 4) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,19?",
             ctrlLocal.adsport,
             indexGroup, indexOffset);
  } else if (lenInPlc == 8) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,8,21?",
             ctrlLocal.adsport,
             indexGroup, indexOffset);
  } else {
    return asynError;
  }
  status = writeReadControllerPrint(traceMask);
  if (status) return asynError;
  nvals = sscanf(inString_, "%u", &iRes);
  if (nvals == 1) {
    *value = iRes;
    asynPrint(pasynUserController_, traceMask,
              "%sout=%s in=%s iRes=0x%x\n",
              modNamEMC, outString_, inString_, iRes);
    return asynSuccess;
  }

  asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
            "%snvals=%d command=\"%s\" response=\"%s\"\n",
            modNamEMC, nvals, outString_, inString_);
  return asynDisabled;
}

asynStatus EthercatMCController::getPlcMemoryString(unsigned indexOffset,
                                                    char *value,
                                                    size_t len)
{
  int traceMask = 0;
  asynStatus status;

  if (ctrlLocal.useADSbinary) {
    memset(value, 0, len);
    return getPlcMemoryViaADS(indexGroup, indexOffset, value, len);
  }
  snprintf(outString_, sizeof(outString_),
           "ADSPORT=%u/.ADR.16#%X,16#%X,%d,30?",
           ctrlLocal.adsport,
           indexGroup, indexOffset, (int)len);
  status = writeReadControllerPrint(traceMask);
  if (status) {
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%scommand=\"%s\" response=\"%s\"\n",
              modNamEMC, outString_, inString_);
    return asynError;
  }

  memcpy(value, inString_, len);
  return asynSuccess;
}

asynStatus EthercatMCController::setPlcMemoryInteger(unsigned indexOffset,
                                                     int value,
                                                     size_t lenInPlc)
{
  int traceMask = 0;
  asynStatus status;

  if (ctrlLocal.useADSbinary) {
    if (lenInPlc == 2) {
      uint8_t raw[2];
      raw[0] = (uint8_t)value;
      raw[1] = (uint8_t)(value >> 8);
      return setPlcMemoryViaADS(indexGroup, indexOffset, raw, sizeof(raw));
    } else if (lenInPlc == 4) {
      uint8_t raw[4];
      raw[0] = (uint8_t)value;
      raw[1] = (uint8_t)(value >> 8);
      raw[2] = (uint8_t)(value >> 16);
      raw[3] = (uint8_t)(value >> 24);
      return setPlcMemoryViaADS(indexGroup, indexOffset, raw, sizeof(raw));
    } else {
      return asynError;
    }
  }

  if (lenInPlc == 2) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,2,2=%d",
             ctrlLocal.adsport,
             indexGroup, indexOffset, value);
  } else if (lenInPlc == 4) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,3=%d",
             ctrlLocal.adsport,
             indexGroup, indexOffset, value);
  } else if (lenInPlc == 8) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,8,20=%d",
             ctrlLocal.adsport,
             indexGroup, indexOffset, value);
  } else {
    return asynError;
  }
  status = writeReadOnErrorDisconnect();
  if (status) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
  asynPrint(pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, outString_, inString_,
            pasynManager->strStatus(status), (int)status);
  if (status) return status;
  return checkACK(outString_, strlen(outString_), inString_);
}


asynStatus EthercatMCController::getPlcMemoryDouble(unsigned indexOffset,
                                                    double *value,
                                                    size_t lenInPlc)
{
  int traceMask = 0;
  int nvals;
  double fRes;
  asynStatus status;


  if (ctrlLocal.useADSbinary) {
    /* This works on little endian boxes only */
    memset(value, 0, lenInPlc);
    if (lenInPlc == 4) {
      float res;
      status = getPlcMemoryViaADS(indexGroup, indexOffset, &res, sizeof(res));
      *value = (double)res;
      return status;
    } else if (lenInPlc == 8) {
      return getPlcMemoryViaADS(indexGroup, indexOffset, value, lenInPlc);
    }
    return asynError;
  }

  if (lenInPlc == 4) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,4?",
             ctrlLocal.adsport,
             indexGroup, indexOffset);
  } else if (lenInPlc == 8) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,8,5?",
             ctrlLocal.adsport,
             indexGroup, indexOffset);
  } else {
    return asynError;
  }
  status = writeReadControllerPrint(traceMask);
  if (status) return status;

  nvals = sscanf(inString_, "%lf", &fRes);
  if (nvals == 1) {
    *value = fRes;
    return asynSuccess;
  }

  asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
            "%snvals=%d command=\"%s\" response=\"%s\"\n",
            modNamEMC, nvals, outString_, inString_);
  return asynDisabled;
}

asynStatus EthercatMCController::setPlcMemoryDouble(unsigned indexOffset,
                                                    double value,
                                                    size_t lenInPlc)
{
  asynStatus status;

  if (ctrlLocal.useADSbinary) {
    if (lenInPlc == 4) {
      float res = (float)value;
      return setPlcMemoryViaADS(indexGroup, indexOffset, &res, sizeof(res));
    } else if (lenInPlc == 8) {
      double res = value;
      return setPlcMemoryViaADS(indexGroup, indexOffset, &res, sizeof(res));
    }
    return asynError;
  }

  if (lenInPlc == 4) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,4=%f",
             ctrlLocal.adsport,
             indexGroup, indexOffset, value);
  } else if (lenInPlc == 8) {
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,8,5=%f",
             ctrlLocal.adsport,
             indexGroup, indexOffset, value);
  } else {
    return asynError;
  }
  status = writeReadOnErrorDisconnect();
  if (status) return status;
  return checkACK(outString_, strlen(outString_), inString_);
}


asynStatus EthercatMCController::readDeviceIndexer(unsigned devNum,
                                                   unsigned infoType)
{
  asynStatus status;
  unsigned value = (devNum + (infoType << 8));
  unsigned valueAcked = 0x8000 + value;
  unsigned counter = 0;
  if (devNum > 0xFF)   return asynDisabled;
  if (infoType > 0xFF) return asynDisabled;

  /* https://forge.frm2.tum.de/public/doc/plc/master/singlehtml/
     The ACK bit on bit 15 must be set when we read back.
     devNum and infoType must match our request as well,
     otherwise there is a collision.
  */
  status = setPlcMemoryInteger(ctrlLocal.indexerOffset, value, 2);
  if (status) return status;
  while (counter < 5) {
    status = getPlcMemoryUint(ctrlLocal.indexerOffset, &value, 2);
    if (status) return status;
    if (value == valueAcked) return asynSuccess;
    counter++;
    epicsThreadSleep(.1 * (counter<<1));
  }
  if (!ctrlLocal.useADSbinary) {
    asynPrint(pasynUserController_,
              ASYN_TRACE_INFO,
              "%sout=%s in=%s (%x) counter=%u\n",
              modNamEMC, outString_, inString_, atoi(inString_),
              counter);
  }
  return asynDisabled;
}

asynStatus EthercatMCController::indexerParamWaitNotBusy(unsigned indexOffset)
{
  unsigned traceMask = ASYN_TRACE_FLOW;
  asynStatus status;
  unsigned   cmdSubParamIndex = 0;
  unsigned   counter = 0;

  while (counter < 5) {
    status = getPlcMemoryUint(indexOffset, &cmdSubParamIndex, 2);
    asynPrint(pasynUserController_,
              status ? traceMask | ASYN_TRACE_INFO : traceMask,
              "%sout=%s in=%s cmdSubParamIndex=0x%04x status=%s (%d)\n",
              modNamEMC, outString_, inString_, cmdSubParamIndex,
              pasynManager->strStatus(status), (int)status);
    if (status) return status;
    switch (cmdSubParamIndex & PARAM_IF_CMD_MASK) {
      case PARAM_IF_CMD_INVALID:
      case PARAM_IF_CMD_DONE:
      case PARAM_IF_CMD_ERR_NO_IDX:
      case PARAM_IF_CMD_READONLY:
      case PARAM_IF_CMD_RETRY_LATER:
        return asynSuccess;
      case PARAM_IF_CMD_BUSY:
        asynPrint(pasynUserController_, ASYN_TRACE_INFO,
                  "%sout=%s in=%s (%x) BUSY\n",
                  modNamEMC, outString_, inString_, atoi(inString_));
        return asynDisabled;
      default:
        ; /* Read, write continue looping */
    }
    counter++;
    epicsThreadSleep(.1 * (counter<<1));
  }
  asynPrint(pasynUserController_, ASYN_TRACE_INFO,
            "%sout=%s in=%s cmdSubParamIndex=0x%04x counter=%d\n",
            modNamEMC, outString_, inString_, cmdSubParamIndex,
            counter);
  return asynDisabled;
}


asynStatus EthercatMCController::indexerParamRead(unsigned paramIfOffset,
                                                  unsigned paramIndex,
                                                  unsigned lenInPlcPara,
                                                  double   *value)
{
  unsigned traceMask = ASYN_TRACE_FLOW;
  asynStatus status;
  unsigned cmd      = PARAM_IF_CMD_DOREAD + paramIndex;
  unsigned cmdAcked = PARAM_IF_CMD_DONE   + paramIndex;
  unsigned lenInPlcCmd = 2;
  unsigned counter = 0;

  if (paramIndex > 0xFF) {
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s paramIndex=%d\n",
              modNamEMC, paramIndex);
    return asynDisabled;
  }
  status = indexerParamWaitNotBusy(paramIfOffset);
  if (status) return status;

  /*
     The parameter interface has this layout:
     0 CmdParamReasonIdx
     2 ParamValue
  */

  status = setPlcMemoryInteger(paramIfOffset, cmd, lenInPlcCmd);
  if (status) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
  asynPrint(pasynUserController_, traceMask,
            "%sout=%s in=%s (%x) status=%s (%d)\n",
            modNamEMC, outString_, inString_, atoi(inString_),
            pasynManager->strStatus(status), (int)status);
  if (status) return status;
  while (counter < 5) {
    unsigned cmdSubParamIndex = 0;
    double fValue;
    if (ctrlLocal.useADSbinary) {
      if (lenInPlcPara == 4) {
        if (paramIndex < 30) {
          /* parameters below 30 are unsigned integers in the PLC
             Read them as integers from PLC, and parse into a double */
          struct {
            uint16_t  paramCtrl;
            uint32_t  paramValue;
          } paramIf;
          status = getPlcMemoryViaADS(indexGroup, paramIfOffset,
                                      &paramIf,
                                      sizeof(paramIf));
          cmdSubParamIndex = paramIf.paramCtrl;
          fValue           = (double)paramIf.paramValue;
        } else {
          struct {
            uint16_t  paramCtrl;
            float     paramValue;
          } paramIf;
          status = getPlcMemoryViaADS(indexGroup, paramIfOffset,
                                      &paramIf,
                                      sizeof(paramIf));
          cmdSubParamIndex = paramIf.paramCtrl;
          fValue           = (double)paramIf.paramValue;
        }
      } else {
        return asynError;
      }
      if (status) return status;
    } else {
      int nvals;
      if (lenInPlcPara == 4) {
        if (paramIndex < 30) {
          /* parameters below 30 are unsigned integers in the PLC
             Read them as integers from PLC, and parse into a double */
          snprintf(outString_, sizeof(outString_),
                   "ADSPORT=%u/.ADR.16#%X,16#%X,2,18?;ADSPORT=%u/.ADR.16#%X,16#%X,4,19?",
                   ctrlLocal.adsport, indexGroup, paramIfOffset,
                   ctrlLocal.adsport, indexGroup, paramIfOffset + lenInPlcCmd);
        } else {
          snprintf(outString_, sizeof(outString_),
                   "ADSPORT=%u/.ADR.16#%X,16#%X,2,18?;ADSPORT=%u/.ADR.16#%X,16#%X,4,4?",
                   ctrlLocal.adsport, indexGroup, paramIfOffset,
                   ctrlLocal.adsport, indexGroup, paramIfOffset + lenInPlcCmd);
        }
      } else if (lenInPlcPara == 8) {
        snprintf(outString_, sizeof(outString_),
                 "ADSPORT=%u/.ADR.16#%X,16#%X,2,18?;ADSPORT=%u/.ADR.16#%X,16#%X,8,5?",
                 ctrlLocal.adsport, indexGroup, paramIfOffset,
                 ctrlLocal.adsport, indexGroup, paramIfOffset + lenInPlcCmd);
      } else {
        asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%snlenInPlcPara=%u\n",
                  modNamEMC, lenInPlcPara);
        return asynError;
      }
      status = writeReadOnErrorDisconnect();
      if (status) return status;
      nvals = sscanf(inString_, "%u;%lf", &cmdSubParamIndex, &fValue);
      if (nvals != 2) {
        traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
        asynPrint(pasynUserController_, traceMask,
                  "%sout=%s in=%s\n",
                  modNamEMC, outString_, inString_);
        return asynError;
      }
    }

    if (cmdSubParamIndex == cmdAcked) {
      /* This is good, return */
      *value = fValue;
      return asynSuccess;
    }

    switch (cmdSubParamIndex & PARAM_IF_CMD_MASK) {
      case PARAM_IF_CMD_INVALID:
        status = asynDisabled;
      case PARAM_IF_CMD_DOREAD:
        status = asynDisabled;
      case PARAM_IF_CMD_DOWRITE:
      case PARAM_IF_CMD_BUSY:
        break;
      case PARAM_IF_CMD_DONE:
        /* This is an error. (collision ?) */
        status = asynDisabled;
      case PARAM_IF_CMD_ERR_NO_IDX:
        status = asynDisabled;
      case PARAM_IF_CMD_READONLY:
        status = asynDisabled;
      case PARAM_IF_CMD_RETRY_LATER:
        status = asynDisabled;
    }
    if (status) {
      traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
      asynPrint(pasynUserController_, traceMask,
                "%sout=%s in=%s cmdSubParamIndex=0x%04x counter=%u status=%s (%d)\n",
                modNamEMC, outString_, inString_, cmdSubParamIndex,
                counter,
                pasynManager->strStatus(status), (int)status);
      return status;
    }
    epicsThreadSleep(.1 * (counter<<1));
    counter++;
  }
  asynPrint(pasynUserController_,
            ASYN_TRACE_INFO,
            "%sout=%s in=%s (%x) counter=%u\n",
            modNamEMC, outString_, inString_, atoi(inString_),
            counter);
  return asynDisabled;
}

asynStatus EthercatMCController::indexerParamWrite(unsigned paramIfOffset,
                                                   unsigned paramIndex,
                                                   double value)
{
  unsigned traceMask = ASYN_TRACE_INFO;
  asynStatus status;
  unsigned cmd      = PARAM_IF_CMD_DOWRITE + paramIndex;
  unsigned cmdAcked = PARAM_IF_CMD_DONE    + paramIndex;
  size_t lenInPlcCmd = 2;
  size_t lenInPlcPara = 4;
  unsigned counter = 0;

  if (paramIndex > 0xFF) return asynDisabled;
  status = indexerParamWaitNotBusy(paramIfOffset);
  if (status) return status;

  /*
     The parameter interface has this layout:
     0 CmdParamReasonIdx
     2 ParamValue
  */
  /* Parameters 1..4 are integers, the rest is floating point */
  if (paramIndex <= 4)
    status = setPlcMemoryInteger(paramIfOffset + 2, (int)value, lenInPlcPara);
  else
    status = setPlcMemoryDouble(paramIfOffset + 2, value, lenInPlcPara);
  if (status) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
  asynPrint(pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, outString_, inString_,
            pasynManager->strStatus(status), (int)status);
  if (status) return status;

  status = setPlcMemoryInteger(paramIfOffset, cmd, lenInPlcCmd);
  if (status) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
  asynPrint(pasynUserController_, traceMask,
            "%sout=%s in=%s (%x) status=%s (%d)\n",
            modNamEMC, outString_, inString_, atoi(inString_),
            pasynManager->strStatus(status), (int)status);
  if (status) return status;
  while (counter < 5) {
    unsigned cmdSubParamIndex = 0;
    status = getPlcMemoryUint(paramIfOffset, &cmdSubParamIndex, 2);
    if (status) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
    asynPrint(pasynUserController_, traceMask,
              "%sout=%s in=%s (%x) cmdSubParamIndex=0x%04x counter=%u status=%s (%d)\n",
              modNamEMC, outString_, inString_, atoi(inString_),
              cmdSubParamIndex, counter,
              pasynManager->strStatus(status), (int)status);
    if (status) return status;
    /* This is good, return */
    if (cmdSubParamIndex == cmdAcked) return asynSuccess;
    switch (cmdSubParamIndex & PARAM_IF_CMD_MASK) {
      case PARAM_IF_CMD_INVALID:
        status = asynDisabled;
      case PARAM_IF_CMD_DOREAD:
        status = asynDisabled;
      case PARAM_IF_CMD_DOWRITE:
      case PARAM_IF_CMD_BUSY:
        break;
      case PARAM_IF_CMD_DONE:
        /* This is an error. (collision ?) */
        status = asynDisabled;
      case PARAM_IF_CMD_ERR_NO_IDX:
        status = asynDisabled;
      case PARAM_IF_CMD_READONLY:
        status = asynDisabled;
      case PARAM_IF_CMD_RETRY_LATER:
        status = asynDisabled;
    }
    if (status) {
      traceMask |= ASYN_TRACE_INFO;
      asynPrint(pasynUserController_, traceMask,
                "%sout=%s in=%s cmdSubParamIndex=0x%04x counter=%u status=%s (%d)\n",
                modNamEMC, outString_, inString_, cmdSubParamIndex,
                counter,
                pasynManager->strStatus(status), (int)status);
      return status;
    }
    epicsThreadSleep(.1 * (counter<<1));
    counter++;
  }
  asynPrint(pasynUserController_,
            traceMask | ASYN_TRACE_INFO,
            "%sout=%s in=%s (%x) counter=%u\n",
            modNamEMC, outString_, inString_, atoi(inString_),
            counter);
  return asynDisabled;
}

void EthercatMCController::parameterFloatReadBack(unsigned axisNo,
                                                  unsigned paramIndex,
                                                  double fValue)
{
  asynMotorAxis *pAxis=getAxis((int)axisNo);
  const static double fullsrev = 200;    /* (default) Full steps/revolution */

  switch(paramIndex) {
  case PARAM_IDX_OPMODE_AUTO_UINT32:
   /* CNEN for EPICS */
   pAxis->setIntegerParam(motorStatusGainSupport_, 1);
   break;
  case PARAM_IDX_MICROSTEPS_FLOAT32:
   pAxis->setDoubleParam(EthercatMCCfgSREV_RB_, fullsrev * fValue);
   break;
  case PARAM_IDX_ABS_MIN_FLOAT32:
    setIntegerParam(axisNo, EthercatMCCfgDLLM_En_, 1);
    pAxis->setDoubleParam(EthercatMCCfgDLLM_, fValue);
#ifdef motorLowLimitROString
    setDoubleParam(motorLowLimitRO_, fValue);
#endif
    break;
  case PARAM_IDX_ABS_MAX_FLOAT32:
    setIntegerParam(axisNo, EthercatMCCfgDHLM_En_, 1);
    pAxis->setDoubleParam(EthercatMCCfgDHLM_, fValue);
#ifdef motorHighLimitROString
    setDoubleParam(motorHighLimitRO_, fValue);
#endif
    break;
  case PARAM_IDX_USR_MIN_FLOAT32:
    break;
  case PARAM_IDX_USR_MAX_FLOAT32:
    break;
  case PARAM_IDX_WRN_MIN_FLOAT32:
    break;
  case PARAM_IDX_WRN_MAX_FLOAT32:
    break;
  case PARAM_IDX_FOLLOWING_ERR_WIN_FLOAT32:
    pAxis->setDoubleParam(EthercatMCCfgPOSLAG_RB_, fValue);
    setIntegerParam(axisNo, EthercatMCCfgPOSLAG_En_RB_, 1);
    break;
  case PARAM_IDX_HYTERESIS_FLOAT32:
    pAxis->setDoubleParam(EthercatMCCfgRDBD_RB_, fValue);
    setIntegerParam(axisNo, EthercatMCCfgRDBD_En_RB_, 1);
#ifdef motorRDBDROString
    pAxis->setDoubleParam(motorRDBDRO_, fValue);
#endif
    break;
  case PARAM_IDX_REFSPEED_FLOAT32:
    pAxis->setDoubleParam(EthercatMCVelToHom_, fValue);
    break;
  case PARAM_IDX_SPEED_FLOAT32:
    pAxis->setDoubleParam(EthercatMCCfgVELO_, fValue);
    pAxis->setDoubleParam(EthercatMCVel_RB_, fValue);
#ifdef motorDefVelocityROString
    pAxis->setDoubleParam(motorDefVelocityRO_, fValue);
#endif
    break;
  case PARAM_IDX_ACCEL_FLOAT32:
    pAxis->setDoubleParam(EthercatMCCfgACCS_, fValue);
    pAxis->setDoubleParam(EthercatMCAcc_RB_, fValue);
#ifdef motorDefJogAccROString
    pAxis->setDoubleParam(motorDefJogAccRO_, fValue);
#endif
    break;
  case PARAM_IDX_IDLE_CURRENT_FLOAT32:
    break;
  case PARAM_IDX_MOVE_CURRENT_FLOAT32:
    break;
  case PARAM_IDX_MICROSTEPS_UINT32:
    break;
  case PARAM_IDX_STEPS_PER_UNIT_FLOAT32:
    {
      double urev = fabs(fullsrev / fValue);
      pAxis->setDoubleParam(EthercatMCCfgUREV_RB_, urev);
    }
    break;
  case PARAM_IDX_HOME_POSITION_FLOAT32:
    pAxis->setDoubleParam(EthercatMCHomPos_, fValue);
    break;
  case PARAM_IDX_FUN_REFERENCE:
#ifdef  motorNotHomedProblemString
    pAxis->setIntegerParam(motorNotHomedProblem_, MOTORNOTHOMEDPROBLEM_ERROR);
#endif
    break;
  }
}

asynStatus
EthercatMCController::indexerReadAxisParameters(EthercatMCIndexerAxis *pAxis,
                                                unsigned devNum, unsigned iOffset)
{
  unsigned axisNo = pAxis->axisNo_;
  unsigned infoType15 = 15;
  asynStatus status;
  unsigned dataIdx;

  status = readDeviceIndexer(devNum, infoType15);
  if (status) {
    return status;
  }
  for (dataIdx = 0; dataIdx <= 16; dataIdx++) {
    unsigned parameters;
    int traceMask = 0;
    parameters = -1;

    status = getPlcMemoryUint(ctrlLocal.indexerOffset + (1 + dataIdx) * 2, &parameters, 2);
    if (status) {
      return status;
    }
      /* dataIdx == 0 has ACK + infoType/devNum
         dataIdx == 1 has supported parameters 15..0 */
    asynPrint(pasynUserController_, traceMask,
              "%sparameters[%03u..%03u]=0x%04x\n",
              modNamEMC, dataIdx*16 +15,
              dataIdx*16, parameters);
    unsigned regSize = 2;
    unsigned indexOffset = iOffset + 5*regSize;
    unsigned bitIdx;

    for (bitIdx = 0; bitIdx <= 15; bitIdx++) {
      unsigned paramIndex = dataIdx*16 + bitIdx;
      unsigned bitIsSet = parameters & (1 << bitIdx) ? 1 : 0;
      if (bitIsSet && (paramIndex < 128)) {
        double fValue = 0;
        unsigned lenInPlcPara = 4;
        status = indexerParamRead(indexOffset,
                                  paramIndex,
                                  lenInPlcPara,
                                  &fValue);
        if (status) {
          return status;
        }
        asynPrint(pasynUserController_, ASYN_TRACE_INFO,
                  "%sparameters(%d) paramIdx=%u fValue=%f\n",
                  modNamEMC, axisNo, paramIndex, fValue);
        parameterFloatReadBack(axisNo, paramIndex, fValue);
      }
    }
  }
  return asynSuccess;
}

asynStatus
EthercatMCController::newIndexerAxis(EthercatMCIndexerAxis *pAxis,
                                     unsigned devNum,
                                     unsigned iAllFlags,
                                     double   fAbsMin,
                                     double   fAbsMax,
                                     unsigned iOffset)
{
  asynStatus status = asynSuccess;
  unsigned axisNo = pAxis->axisNo_;

  pAxis->setStringParam(EthercatMCreason11_, "High limit");
  pAxis->setStringParam(EthercatMCreason10_, "Low limit");
  pAxis->setStringParam(EthercatMCreason9_,  "Dynamic problem, timeout");
  pAxis->setStringParam(EthercatMCreason8_,  "Static problem, inhibit");
#if 0
  pAxis->setStringParam(EthercatMCaux7_,  "Aux 7");
  pAxis->setStringParam(EthercatMCaux6_,  "Aux 6");
  pAxis->setStringParam(EthercatMCaux5_,  "Aux 5");
  pAxis->setStringParam(EthercatMCaux4_,  "Aux 4");
  pAxis->setStringParam(EthercatMCaux3_,  "Aux 3");
  pAxis->setStringParam(EthercatMCaux2_,  "Aux 2");
  pAxis->setStringParam(EthercatMCaux1_,  "Aux 1");
  pAxis->setStringParam(EthercatMCaux0_,  "Aux 0");
#endif
  /* AUX bits */
  {
    unsigned auxBitIdx = 0;
    for (auxBitIdx = 0; auxBitIdx < 23; auxBitIdx++) {
      if ((iAllFlags >> auxBitIdx) & 1) {
        char auxBitName[34];
        unsigned infoType16 = 16;
        memset(&auxBitName, 0, sizeof(auxBitName));
        status = readDeviceIndexer(devNum, infoType16 + auxBitIdx);
        if (status) return status;
        status = getPlcMemoryString(ctrlLocal.indexerOffset + 1*2,
                                      auxBitName,
                                      sizeof(auxBitName));
        asynPrint(pasynUserController_, ASYN_TRACE_INFO,
                  "%sauxBitName[%d] auxBitName(%02u)=%s\n",
                  modNamEMC, axisNo, auxBitIdx, auxBitName);
        if (status) return status;
        pAxis->setStringParam(EthercatMCaux0_ + auxBitIdx, auxBitName);
      }
    }
  }
  /* Unit code */
  {
    int validSoftlimits = fAbsMax > fAbsMin;
    if (fAbsMin <= -3.0e+38 && fAbsMax >= 3.0e+38)
      validSoftlimits = 0;

    /* Soft limits */
    /*  absolute values become read only limits */
    pAxis->setIntegerParam(EthercatMCCfgDHLM_En_, validSoftlimits);
    pAxis->setDoubleParam( EthercatMCCfgDHLM_,    fAbsMax);
    pAxis->setIntegerParam(EthercatMCCfgDLLM_En_, validSoftlimits);
    pAxis->setDoubleParam( EthercatMCCfgDLLM_,    fAbsMin);
#ifdef motorHighLimitROString
    if (validSoftlimits) {
      pAxis->setDoubleParam(motorHighLimitRO_, fAbsMax);
      pAxis->setDoubleParam(motorLowLimitRO_,  fAbsMin);
    }
#endif
  }
  return status;
}

asynStatus EthercatMCController::initialPollIndexer(void)
{
  asynStatus status;
  uint32_t iTmpVer = 0;
  double version = 0.0;
  struct {
    char desc[34];
    char vers[34];
    char author1[34];
    char author2[34];
  } descVersAuthors;
  unsigned devNum;
  unsigned infoType0 = 0;
  unsigned infoType4 = 4;
  unsigned infoType5 = 5;
  unsigned infoType6 = 6;
  unsigned infoType7 = 7;
  int      axisNo = 0;


  {
    uint8_t resss[0x338];
    getSymbolInfoViaADS("Main.M1.nMotionAxisID",
                        &resss, sizeof(resss));
  }

  memset(&descVersAuthors, 0, sizeof(descVersAuthors));
  if (!ctrlLocal.adsport) {
    ctrlLocal.adsport = 851;
  }
  status = getPlcMemoryUint(ctrlLocal.indexerOffset, &iTmpVer, sizeof(iTmpVer));
  if (status) return status;

  if (iTmpVer == 0x44fbe0a4) {
    version = 2015.02;
  }
  asynPrint(pasynUserController_, ASYN_TRACE_INFO,
            "%sadsport=%u version=%f\n",
            modNamEMC, ctrlLocal.adsport, version);

  if (!version) status = asynDisabled;
  if (status) goto endPollIndexer;

  ctrlLocal.indexerOffset = 4;
  status = getPlcMemoryUint(ctrlLocal.indexerOffset, &ctrlLocal.indexerOffset, 2);
  asynPrint(pasynUserController_, ASYN_TRACE_INFO,
            "%sindexerOffset=%u\n",
            modNamEMC, ctrlLocal.indexerOffset);

  for (devNum = 0; devNum < 100; devNum++) {
    unsigned iTypCode = -1;
    unsigned iSize = -1;
    unsigned iOffset = -1;
    unsigned iUnit = -1;
    unsigned iAllFlags = -1;
    double fAbsMin = 0;
    double fAbsMax = 0;
    status = readDeviceIndexer(devNum, infoType0);
    if (!status) {
      if (ctrlLocal.useADSbinary) {
        struct {
          uint8_t   typCode_0;
          uint8_t   typCode_1;
          uint8_t   size_0;
          uint8_t   size_1;
          uint8_t   offset_0;
          uint8_t   offset_1;
          uint8_t   unit_0;
          uint8_t   unit_1;
          uint8_t   flags_0;
          uint8_t   flags_1;
          uint8_t   flags_2;
          uint8_t   flags_3;
          float     absMin;
          float     absMax;
        } infoType0_data;
        status = getPlcMemoryViaADS(indexGroup, ctrlLocal.indexerOffset +  1*2,
                                    &infoType0_data, sizeof(infoType0_data));
        if (!status) {
          iTypCode  = infoType0_data.typCode_0 + (infoType0_data.typCode_1 << 8);
          iSize     = infoType0_data.size_0 + (infoType0_data.size_1 << 8);
          iOffset   = infoType0_data.offset_0 + (infoType0_data.offset_1 << 8);
          iUnit     = infoType0_data.unit_0 + (infoType0_data.unit_1 << 8);
          iAllFlags = infoType0_data.flags_0 + (infoType0_data.flags_1 << 8) +
                      (infoType0_data.flags_2 << 16) + (infoType0_data.flags_3 << 24);
          fAbsMin   = (double)infoType0_data.absMin;
          fAbsMax   = (double)infoType0_data.absMax;
        }
      } else {
        unsigned iFlagsLow = -1;
        unsigned iFlagsHigh = -1;
        getPlcMemoryUint(ctrlLocal.indexerOffset +  1*2, &iTypCode, 2);
        getPlcMemoryUint(ctrlLocal.indexerOffset +  2*2, &iSize, 2);
        getPlcMemoryUint(ctrlLocal.indexerOffset +  3*2, &iOffset, 2);
        getPlcMemoryUint(ctrlLocal.indexerOffset +  4*2, &iUnit, 2);
        getPlcMemoryUint(ctrlLocal.indexerOffset +  5*2, &iFlagsLow, 2);
        getPlcMemoryUint(ctrlLocal.indexerOffset +  6*2, &iFlagsHigh, 2);
        getPlcMemoryDouble(ctrlLocal.indexerOffset  +  7*2, &fAbsMin, 4);
        getPlcMemoryDouble(ctrlLocal.indexerOffset  +  9*2, &fAbsMax, 4);
        iAllFlags = iFlagsLow + (iFlagsHigh << 16);
      }
    }
    status = readDeviceIndexer(devNum, infoType4);
    if (!status) {
      getPlcMemoryString(ctrlLocal.indexerOffset + 1*2,
                              descVersAuthors.desc,
                              sizeof(descVersAuthors.desc));
    }
    status = readDeviceIndexer(devNum, infoType5);
    if (!status) {
      getPlcMemoryString(ctrlLocal.indexerOffset + 1*2,
                         descVersAuthors.vers,
                         sizeof(descVersAuthors.vers));
    }
    status = readDeviceIndexer(devNum, infoType6);
    if (!status) {
      getPlcMemoryString(ctrlLocal.indexerOffset + 1*2,
                              descVersAuthors.author1,
                              sizeof(descVersAuthors.author1));
    }
    status = readDeviceIndexer(devNum, infoType7);
    if (!status) {
      getPlcMemoryString(ctrlLocal.indexerOffset + 1*2,
                         descVersAuthors.author2,
                         sizeof(descVersAuthors.author2));
    }
    asynPrint(pasynUserController_, ASYN_TRACE_INFO,
              "%sindexerDevice Offset=%u %20s "
              "TypCode=0x%x Size=%u UnitCode=0x%x AllFlags=0x%x AbsMin=%e AbsMax=%e\n",
              modNamEMC, iOffset, descVersAuthors.desc,
              iTypCode, iSize, iUnit, iAllFlags, fAbsMin, fAbsMax);
    asynPrint(pasynUserController_, ASYN_TRACE_INFO,
              "%sdescVersAuthors(%d)  vers=%s author1=%s author2=%s\n",
              modNamEMC, axisNo,
              descVersAuthors.vers,
              descVersAuthors.author1,
              descVersAuthors.author2);
    if (!iTypCode && !iSize && !iOffset) {
      break; /* End of list ?? */
    }
    switch (iTypCode) {
      case 0x5008:
      case 0x500C:
        {
          char unitCodeTxt[40];
          EthercatMCIndexerAxis *pAxis;
          axisNo++;
          pAxis = static_cast<EthercatMCIndexerAxis*>(asynMotorController::getAxis(axisNo));
          if (!pAxis) {
            pAxis = new EthercatMCIndexerAxis(this, axisNo);
          }
          /* Now we have an axis */

          status = newIndexerAxis(pAxis,
                                  devNum,
                                  iAllFlags,
                                  fAbsMin,
                                  fAbsMax,
                                  iOffset);
          asynPrint(pasynUserController_, ASYN_TRACE_INFO,
                    "%sTypeCode(%d) iTypCode=%x pAxis=%p\n",
                    modNamEMC, axisNo, iTypCode, pAxis);
          if (status) goto endPollIndexer;

          pAxis->setIndexerDevNumOffsetTypeCode(devNum, iOffset, iTypCode);
          setStringParam(axisNo,  EthercatMCCfgDESC_RB_, descVersAuthors.desc);
          snprintf(unitCodeTxt, sizeof(unitCodeTxt), "%s%s",
          plcUnitPrefixTxt(( (int8_t)((iUnit & 0xFF00)>>8))),
          plcUnitTxtFromUnitCode(iUnit & 0xFF));
          setStringParam(axisNo,  EthercatMCCfgEGU_RB_, unitCodeTxt);
        }
     }
  }

 endPollIndexer:
  /* Special case: asynDisabled means "no indexer found".
     That is OK, return asynSuccess */
  if (status == asynDisabled)
    return asynSuccess;
  return status;

}
