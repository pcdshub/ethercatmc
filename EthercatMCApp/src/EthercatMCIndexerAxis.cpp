/*
  FILENAME... EthercatMCIndexerAxis.cpp
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>

#include <epicsThread.h>

#include "motor.h"
#include "EthercatMC.h"
#include "EthercatMCIndexerAxis.h"

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

static unsigned indexGroup = 0x4020;

typedef enum {
  idxStatusCodeRESET    = 0,
  idxStatusCodeIDLE     = 1,
  idxStatusCodePOWEROFF = 2,
  idxStatusCodeWARN     = 3,
  idxStatusCodeERR4     = 4,
  idxStatusCodeSTART    = 5,
  idxStatusCodeBUSY     = 6,
  idxStatusCodeSTOP     = 7,
  idxStatusCodeERROR    = 8,
  idxStatusCodeERR9     = 9,
  idxStatusCodeERR10    = 10,
  idxStatusCodeERR11    = 11,
  idxStatusCodeERR12    = 12,
  idxStatusCodeERR13    = 13,
  idxStatusCodeERR14    = 14,
  idxStatusCodeERR15    = 15
} idxStatusCodeType;

extern "C" const char *idxStatusCodeTypeToStr(idxStatusCodeType idxStatusCode)
{
  switch (idxStatusCode) {
    case  idxStatusCodeRESET:    return "RESET";
    case  idxStatusCodeIDLE:     return "IDLE ";
    case  idxStatusCodePOWEROFF: return "PWROF";
    case  idxStatusCodeWARN:     return "WARN ";
    case  idxStatusCodeERR4:     return "ERR4 ";
    case  idxStatusCodeSTART:    return "START";
    case  idxStatusCodeBUSY:     return "BUSY ";
    case  idxStatusCodeSTOP:     return "STOP ";
    case  idxStatusCodeERROR:    return "ERROR";
    case  idxStatusCodeERR9:     return "ERR9 ";
    case  idxStatusCodeERR10:    return "ERR10";
    case  idxStatusCodeERR11:    return "ERR11";
    case  idxStatusCodeERR12:    return "ERR12";
    case  idxStatusCodeERR13:    return "ERR13";
    case  idxStatusCodeERR14:    return "ERR14";
    case  idxStatusCodeERR15:    return "ERR15";
    default:                     return "UKNWN";
  }
}


//
// These are the EthercatMCIndexerAxis methods
//

/** Creates a new EthercatMCIndexerAxis object.
 * \param[in] pC Pointer to the EthercatMCController to which this axis belongs.
 * \param[in] axisNo Index number of this axis, range 1 to pC->numAxes_. (0 is not used)
 *
 *
 * Initializes register numbers, etc.
 */
EthercatMCIndexerAxis::EthercatMCIndexerAxis(EthercatMCController *pC, int axisNo)
  : asynMotorAxis(pC, axisNo),
    pC_(pC)
{
#ifdef motorFlagsDriverUsesEGUString
    setIntegerParam(pC_->motorFlagsDriverUsesEGU_,1);
#endif
#ifdef motorFlagsAdjAfterHomedString
    setIntegerParam(pC_->motorFlagsAdjAfterHomed_, 1);
#endif
  memset(&drvlocal, 0, sizeof(drvlocal));
  memset(&drvlocal.dirty, 0xFF, sizeof(drvlocal.dirty));
  /* We pretend to have an encoder (fActPosition) */
  setIntegerParam(pC_->motorStatusHasEncoder_, 1);
#ifdef motorFlagsNoStopProblemString
  setIntegerParam(pC_->motorFlagsNoStopProblem_, 1);
#endif
#ifdef motorFlagsNoStopOnLsString
  setIntegerParam(pC_->motorFlagsNoStopOnLS_, 1);
#endif
#ifdef motorFlagsLSrampDownString
  setIntegerParam(pC_->motorFlagsLSrampDown_, 1);
#endif
#ifdef motorFlagsPwrWaitForOnString
  setIntegerParam(pC_->motorFlagsPwrWaitForOn_, 1);
#endif

#ifdef motorShowPowerOffString
    setIntegerParam(pC_->motorShowPowerOff_, 1);
#endif
  drvlocal.scaleFactor = 1.0;
  /* Set the module name to "" if we have FILE/LINE enabled by asyn */
  if (pasynTrace->getTraceInfoMask(pC_->pasynUserController_) & ASYN_TRACEINFO_SOURCE) modNamEMC = "";
}

extern "C" int EthercatMCCreateIndexerAxis(const char *EthercatMCName, int axisNo,
                                           int axisFlags, const char *axisOptionsStr)
{
#if 0
  EthercatMCController *pC;

  pC = (EthercatMCController*) findAsynPortDriver(EthercatMCName);
  if (!pC)
  {
    printf("Error port %s not found\n", EthercatMCName);
    return asynError;
  }
  pC->lock();
  new EthercatMCIndexerAxis(pC, axisNo, axisFlags, axisOptionsStr);
  pC->unlock();
#endif
  return asynSuccess;
}

void EthercatMCIndexerAxis::setIndexerDevNumOffsetTypeCode(unsigned devNum,
                                                           unsigned iOffset,
                                                           unsigned iTypCode)
{
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%s (%d) devNum=%u iTypCode=0x%x, iOffset=%u\n",
            modNamEMC, axisNo_, devNum, iTypCode, iOffset);
  drvlocal.devNum = devNum;
  drvlocal.iTypCode = iTypCode;
  drvlocal.iOffset = iOffset;
}


/** Reports on status of the axis
 * \param[in] fp The file pointer on which report information will be written
 * \param[in] level The level of report detail desired
 *
 * After printing device-specific information calls asynMotorAxis::report()
 */
void EthercatMCIndexerAxis::report(FILE *fp, int level)
{
  if (level > 0) {
    fprintf(fp, "  axis %d\n", axisNo_);
  }

  // Call the base class method
  asynMotorAxis::report(fp, level);
}




/** Move the axis to a position, either absolute or relative
 * \param[in] position in steps
 * \param[in] relative (0=absolute, otherwise relative)
 * \param[in] minimum velocity, steps/sec
 * \param[in] maximum velocity, steps/sec
 * \param[in] acceleration,  steps/sec/sec
 *
 */
asynStatus EthercatMCIndexerAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status = asynSuccess;
  unsigned traceMask = ASYN_TRACE_INFO;
    /* param devices look like this
       0x0 Actual value,   32 bit float
       0x4 Setpoint        32 bit float
       0x8 CmdStatusReason 16 bit integer/bitwise
       0xA ParamCmd        16 bit integer/bitwise
       0xE ParamValue      32 bit float or integer
    */
  unsigned cmdReason = idxStatusCodeSTART  << 12;
  unsigned paramIfOffset = drvlocal.iOffset + 0xA;
  asynPrint(pC_->pasynUserController_, traceMask,
            "%smove (%d) position=%f relative=%d minVelocity=%f maxVelocity=%f acceleration=%f\n",
            "EthercatMCIndexerAxis", axisNo_,
            position, relative, minVelocity, maxVelocity, acceleration);
  if (maxVelocity > 0.0) {
    double oldValue;
    pC_->getDoubleParam(axisNo_, pC_->EthercatMCVel_RB_, &oldValue);
    if (maxVelocity != oldValue) {
      status = pC_->indexerParamWrite(paramIfOffset,
                                      PARAM_IDX_SPEED_FLOAT32, maxVelocity);
      if (status) {
        asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%smove (%d) status=%s (%d)\n",
                  "EthercatMCIndexerAxis", axisNo_,
                  pasynManager->strStatus(status), (int)status);
        return status;
      }
      setDoubleParam(pC_->EthercatMCVel_RB_, maxVelocity);
    }
  }
  if (acceleration > 0.0) {
    double oldValue;
    pC_->getDoubleParam(axisNo_, pC_->EthercatMCAcc_RB_, &oldValue);
    if (acceleration != oldValue) {
      status = pC_->indexerParamWrite(paramIfOffset,
                                      PARAM_IDX_ACCEL_FLOAT32, acceleration);
      if (status) {
        asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%smove (%d) status=%s (%d)\n",
                  "EthercatMCIndexerAxis", axisNo_,
                  pasynManager->strStatus(status), (int)status);
        return status;
      }
      setDoubleParam(pC_->EthercatMCAcc_RB_, acceleration);
    }
  }
  if (relative){
    double actPosition;
    pC_->getDoubleParam(axisNo_, pC_->motorPosition_, &actPosition);
    position = position - actPosition;
  }
  snprintf(pC_->outString_, sizeof(pC_->outString_),
           "ADSPORT=%u/.ADR.16#%X,16#%X,4,4=%f;ADSPORT=%u/.ADR.16#%X,16#%X,2,2=%d",
           pC_->adsport, indexGroup, drvlocal.iOffset + 4, position,
           pC_->adsport, indexGroup, drvlocal.iOffset + 8, cmdReason);
  return pC_->writeReadACK(traceMask);
}


/** Home the motor, search the home position
 * \param[in] minimum velocity, mm/sec
 * \param[in] maximum velocity, mm/sec
 * \param[in] acceleration, seconds to maximum velocity
 * \param[in] forwards (0=backwards, otherwise forwards)
 *
 */
asynStatus EthercatMCIndexerAxis::home(double minVelocity, double maxVelocity, double acceleration, int forwards)
{
  asynStatus status;
  unsigned paramIfOffset = drvlocal.iOffset + 0xA;
  (void)minVelocity;
  (void)maxVelocity;
  (void)acceleration;
  (void)forwards;
  status = pC_->indexerParamWrite(paramIfOffset,
                                  PARAM_IDX_FUN_REFERENCE,
                                  0.0);
  return status;
}


/** jog the the motor, search the home position
 * \param[in] minimum velocity, mm/sec (not used)
 * \param[in] maximum velocity, mm/sec (positive or negative)
 * \param[in] acceleration, seconds to maximum velocity
 *
 */
asynStatus EthercatMCIndexerAxis::moveVelocity(double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status;
  unsigned paramIfOffset = drvlocal.iOffset + 0xA;
  (void)minVelocity;
  (void)acceleration;
  if (acceleration > 0.0) {
    double oldValue;
    pC_->getDoubleParam(axisNo_, pC_->EthercatMCAcc_RB_, &oldValue);
    if (acceleration != oldValue) {
      status = pC_->indexerParamWrite(paramIfOffset,
                                      PARAM_IDX_ACCEL_FLOAT32, acceleration);
      if (status) {
        asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%smoveVelocity (%d) status=%s (%d)\n",
                  "EthercatMCIndexerAxis", axisNo_,
                  pasynManager->strStatus(status), (int)status);
        return status;
      }
      setDoubleParam(pC_->EthercatMCAcc_RB_, acceleration);
    }
  }
  status = pC_->indexerParamWrite(paramIfOffset,
                                  PARAM_IDX_FUN_MOVE_VELOCITY,
                                  maxVelocity);
  return status;
}


/**
 * See asynMotorAxis::setPosition
 */
asynStatus EthercatMCIndexerAxis::setPosition(double value)
{
  asynStatus status = asynSuccess;
  return status;
}

/** Stop the axis
 *
 */
asynStatus EthercatMCIndexerAxis::stopAxisInternal(const char *function_name, double acceleration)
{
  asynStatus status = asynSuccess;
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sstopAxisInternal(%d) (%s)\n", modNamEMC, axisNo_, function_name);

  size_t lenInPlc = 2;
  unsigned reg = 4;
  unsigned traceMask = ASYN_TRACE_INFO;
  unsigned cmdReason = idxStatusCodeSTOP << 12;
  status = pC_->setPlcMemoryInteger(drvlocal.iOffset + reg * lenInPlc,
                                         cmdReason, lenInPlc);
  asynPrint(pC_->pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, pC_->outString_, pC_->inString_,
            pasynManager->strStatus(status), (int)status);
  return status;
}

/** Stop the axis, called by motor Record
 *
 */
asynStatus EthercatMCIndexerAxis::stop(double acceleration )
{
  //drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
  return stopAxisInternal(__FUNCTION__, acceleration);
}

/** Polls the axis.
 * This function reads the motor position, the limit status, the home status, the moving status,
 * and the drive power-on status.
 * It calls setIntegerParam() and setDoubleParam() for each item that it polls,
 * and then calls callParamCallbacks() at the end.
 * \param[out] moving A flag that is set indicating that the axis is moving (true) or done (false). */
asynStatus EthercatMCIndexerAxis::poll(bool *moving)
{
  static unsigned indexGroup = 0x4020;
  asynStatus status = asynSuccess;
  if (drvlocal.iTypCode && drvlocal.iOffset) {
    int nvals;
    unsigned traceMask = ASYN_TRACE_INFO;
    double targetPosition = 0.0;
    double actPosition = 0.0;
    double paramValue = 0.0;
    unsigned statusReasonAux, paramCtrl;
    bool nowMoving = false;
    int powerIsOn = 1; /* Unless powerOff */
    int statusValid = 0;
    idxStatusCodeType idxStatusCode;
    int pollReadBackInBackGround = 0;
    if (drvlocal.dirty.initialPollNeeded) {
      status = pC_->indexerReadAxisParameters(this, drvlocal.devNum, drvlocal.iOffset);
      if (!status) {
        drvlocal.dirty.initialPollNeeded = 0;
        setIntegerParam(pC_->motorStatusCommsError_, 0);
      }
    }

    snprintf(pC_->outString_, sizeof(pC_->outString_),
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,4?;"
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,4?;"
             "ADSPORT=%u/.ADR.16#%X,16#%X,2,18?;"
             "ADSPORT=%u/.ADR.16#%X,16#%X,2,18?;"
             "ADSPORT=%u/.ADR.16#%X,16#%X,4,4?",
             pC_->adsport,
             indexGroup, drvlocal.iOffset,         /* Actual value on pos 0 */
             pC_->adsport,
             indexGroup, drvlocal.iOffset + 4,     /* Target value on pos 4 */
             pC_->adsport,
             indexGroup, drvlocal.iOffset + 8,     /* CmdStatusAux value on pos 8 */
             pC_->adsport,
             indexGroup, drvlocal.iOffset + 0xA,   /* ParamCtl on 0xA*/
             pC_->adsport,
             indexGroup, drvlocal.iOffset + 0xC);  /* ParamValue on 0xC*/

    status = pC_->writeReadOnErrorDisconnect();
    if (status) {
      return status;
    }
    nvals = sscanf(pC_->inString_, "%lf;%lf;%u;%u;%lf",
                   &actPosition, &targetPosition, &statusReasonAux,
                   &paramCtrl, &paramValue);
    if (nvals != 5) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%snvals=%d command=\"%s\" response=\"%s\"\n",
                modNamEMC, nvals, pC_->outString_, pC_->inString_);
      return asynError;
    }

    setDoubleParam(pC_->motorPosition_, actPosition);
    drvlocal.hasProblem = 0;
    idxStatusCode = (idxStatusCodeType)(statusReasonAux >> 12);
    setIntegerParam(pC_->EthercatMCStatusCode_, idxStatusCode);
    if (statusReasonAux != drvlocal.old_statusReasonAux) {
      asynPrint(pC_->pasynUserController_, traceMask,
                "%spoll(%d) actPos=%f targetPos=%f statusReasonAux=0x%x %d (%s)\n",
                modNamEMC, axisNo_,
                actPosition, targetPosition,
                statusReasonAux, idxStatusCode,
                idxStatusCodeTypeToStr(idxStatusCode));
      drvlocal.old_statusReasonAux = statusReasonAux;
    }
    if ((paramCtrl != drvlocal.old_paramCtrl) ||
        (paramValue != drvlocal.old_paramValue)) {
      asynPrint(pC_->pasynUserController_,
                pollReadBackInBackGround ? ASYN_TRACE_FLOW : ASYN_TRACE_INFO,
                "%spoll(%d) paramCtrl=%x paramValue=%f\n",
                modNamEMC, axisNo_,
                paramCtrl, paramValue);
      if ((paramCtrl & PARAM_IF_CMD_MASK) == PARAM_IF_CMD_DONE) {
        pC_->parameterFloatReadBack(axisNo_,
                                    paramCtrl & PARAM_IF_IDX_MASK,
                                    paramValue);
      }
      drvlocal.old_paramCtrl = paramCtrl;
      drvlocal.old_paramValue = paramValue;
    }
    switch (idxStatusCode) {
      /* After RESET, START, STOP the bits are not valid */
    case idxStatusCodeIDLE:
    case idxStatusCodeWARN:
      statusValid = 1;
      break;
    case idxStatusCodePOWEROFF:
      statusValid = 1;
      drvlocal.hasProblem = 1;
      powerIsOn = 0;
      break;
    case idxStatusCodeBUSY:
      statusValid = 1;
      nowMoving = true;
      break;
    case idxStatusCodeERROR:
      statusValid = 1;
      drvlocal.hasProblem = 1;
      break;
    default:
      drvlocal.hasProblem = 1;
    }
    if (statusValid) {
      int hls = statusReasonAux & 0x0800 ? 1 : 0;
      int lls = statusReasonAux & 0x0400 ? 1 : 0;
      setIntegerParam(pC_->motorStatusLowLimit_, lls);
      setIntegerParam(pC_->motorStatusHighLimit_, hls);
      setIntegerParam(pC_->motorStatusMoving_, nowMoving);
      setIntegerParam(pC_->motorStatusDone_, !nowMoving);
      setIntegerParam(pC_->EthercatMCStatusBits_, statusReasonAux & 0xFFF);
    }
    *moving = nowMoving;
    setIntegerParam(pC_->EthercatMCStatusCode_, idxStatusCode);
    setIntegerParam(pC_->motorStatusProblem_, drvlocal.hasProblem);
    setIntegerParam(pC_->motorStatusPowerOn_, powerIsOn);
    /* Read back the parameters one by one */
    if (pollReadBackInBackGround &&
        !nowMoving && (paramCtrl & PARAM_IF_ACK_MASK)) {
      unsigned newParamCtrl;
      drvlocal.pollNowIdx++;
      if (drvlocal.pollNowIdx >= sizeof(pollNowParams)/sizeof(pollNowParams[0])) {
        drvlocal.pollNowIdx = 0;
      }
      newParamCtrl = PARAM_IF_CMD_DOREAD + pollNowParams[drvlocal.pollNowIdx];
      snprintf(pC_->outString_, sizeof(pC_->outString_),
               "ADSPORT=%u/.ADR.16#%X,16#%X,2,18=%d",
               pC_->adsport,
               indexGroup, drvlocal.iOffset + 0xA,   /* ParamCtl on 0xA*/
               newParamCtrl
               );
      pC_->writeReadACK(ASYN_TRACE_FLOW);
    }
    callParamCallbacks();
  }
  return status;
}


asynStatus EthercatMCIndexerAxis::resetAxis(void)
{
  asynStatus status = asynSuccess;
  unsigned reg = 4;
  size_t lenInPlc = 2;
  unsigned traceMask = ASYN_TRACE_INFO;
  unsigned cmdReason = idxStatusCodeRESET << 12;
  status = pC_->setPlcMemoryInteger(drvlocal.iOffset + reg * lenInPlc,
                                    cmdReason, lenInPlc);
  asynPrint(pC_->pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, pC_->outString_, pC_->inString_,
            pasynManager->strStatus(status), (int)status);
  return status;
}

/** Set the motor closed loop status
  * \param[in] closedLoop true = close loop, false = open looop. */
asynStatus EthercatMCIndexerAxis::setClosedLoop(bool closedLoop)
{
  double value = closedLoop ? 0.0 : 1.0; /* 1.0 means disable */
  unsigned paramIfOffset = drvlocal.iOffset + 0xA;
  asynStatus status;
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%ssetClosedLoop(%d)=%d\n",  modNamEMC, axisNo_,
            (int)closedLoop);
  status = pC_->indexerParamWrite(paramIfOffset,
                                  PARAM_IDX_OPMODE_AUTO_UINT32, value);
  return asynSuccess;
}


asynStatus EthercatMCIndexerAxis::setIntegerParam(int function, int value)
{
  asynStatus status;
  if (function == pC_->motorUpdateStatus_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetIntegerParam(%d motorUpdateStatus_)=%d\n", modNamEMC, axisNo_, value);

  } else if (function == pC_->motorStatusCommsError_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_FLOW,
              "%ssetIntegerParam(%d pC_->motorStatusCommsError_)=%d\n",
              modNamEMC, axisNo_, value);
    if (value && !drvlocal.dirty.oldStatusDisconnected) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%s Communication error(%d)\n", modNamEMC, axisNo_);
      memset(&drvlocal.dirty, 0xFF, sizeof(drvlocal.dirty));
      //drvlocal.MCU_nErrorId = 0;
    }
#ifdef EthercatMCErrRstString
  } else if (function == pC_->EthercatMCErrRst_) {
    if (value) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
                "%ssetIntegerParam(%d ErrRst_)=%d\n",
                modNamEMC, axisNo_, value);
      /*  We do not want to call the base class */
      return resetAxis();
    }
    /* If someone writes 0 to the field, just ignore it */
    return asynSuccess;
#endif
  }
  //Call base class method
  status = asynMotorAxis::setIntegerParam(function, value);
  return status;
}
