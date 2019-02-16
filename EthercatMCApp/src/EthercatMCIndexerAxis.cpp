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

#define WAITNUMPOLLSBEFOREREADY 3

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
EthercatMCIndexerAxis::EthercatMCIndexerAxis(EthercatMCController *pC, int axisNo,
                                             int axisFlags, const char *axisOptionsStr)
  : EthercatMCBaseAxis(pC, axisNo),
    pC_(pC)
{
#ifdef motorFlagsDriverUsesEGUString
    setIntegerParam(pC_->motorFlagsDriverUsesEGU_,1);
#endif
#ifdef motorFlagsAdjAfterHomedString
    setIntegerParam(pC_->motorFlagsAdjAfterHomed_, 1);
#endif
#ifdef motorWaitPollsBeforeReadyString
  setIntegerParam(pC_->motorWaitPollsBeforeReady_ , WAITNUMPOLLSBEFOREREADY);
#endif
  memset(&drvlocal, 0, sizeof(drvlocal));
  memset(&drvlocal.dirty, 0xFF, sizeof(drvlocal.dirty));
  //drvlocal.old_eeAxisError = eeAxisErrorIOCcomError;
  //drvlocal.axisFlags = axisFlags;

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
#ifdef  motorNotHomedProblemString
    setIntegerParam(pC_->motorNotHomedProblem_, MOTORNOTHOMEDPROBLEM_ERROR);
#endif

  drvlocal.scaleFactor = 1.0;
  if (axisFlags & AMPLIFIER_ON_FLAG_USING_CNEN) {
    setIntegerParam(pC->motorStatusGainSupport_, 1);
  }
  if (axisOptionsStr && axisOptionsStr[0]) {
    const char * const encoder_is_str = "encoder=";
    const char * const cfgfile_str = "cfgFile=";
    const char * const cfgDebug_str = "getDebugText=";
#ifndef motorFlagsDriverUsesEGUString
    const char * const stepSize_str = "stepSize=";
#endif
    const char * const homProc_str = "HomProc=";
    const char * const homPos_str  = "HomPos=";
    const char * const adsPort_str  = "adsPort=";
    const char * const scaleFactor_str = "scaleFactor=";

    char *pOptions = strdup(axisOptionsStr);
    char *pThisOption = pOptions;
    char *pNextOption = pOptions;

    while (pNextOption && pNextOption[0]) {
      pNextOption = strchr(pNextOption, ';');
      if (pNextOption) {
        *pNextOption = '\0'; /* Terminate */
        pNextOption++;       /* Jump to (possible) next */
      }
      if (!strncmp(pThisOption, encoder_is_str, strlen(encoder_is_str))) {
        pThisOption += strlen(encoder_is_str);
        drvlocal.externalEncoderStr = strdup(pThisOption);
      }  else if (!strncmp(pThisOption, cfgfile_str, strlen(cfgfile_str))) {
        pThisOption += strlen(cfgfile_str);
        //drvlocal.cfgfileStr = strdup(pThisOption);
      } else if (!strncmp(pThisOption, cfgDebug_str, strlen(cfgDebug_str))) {
        pThisOption += strlen(cfgDebug_str);
        //drvlocal.cfgDebug_str = strdup(pThisOption);
#ifndef motorFlagsDriverUsesEGUString
      } else if (!strncmp(pThisOption, stepSize_str, strlen(stepSize_str))) {
        pThisOption += strlen(stepSize_str);
        /* This option is obsolete, depending on motor */
        drvlocal.scaleFactor = atof(pThisOption);
#endif
      } else if (!strncmp(pThisOption, adsPort_str, strlen(adsPort_str))) {
        pThisOption += strlen(adsPort_str);
        int adsPort = atoi(pThisOption);
        if (adsPort > 0) {
          drvlocal.adsPort = (unsigned)adsPort;
        }
      } else if (!strncmp(pThisOption, homProc_str, strlen(homProc_str))) {
        pThisOption += strlen(homProc_str);
        int homProc = atoi(pThisOption);
        setIntegerParam(pC_->EthercatMCHomProc_, homProc);
      } else if (!strncmp(pThisOption, homPos_str, strlen(homPos_str))) {
        pThisOption += strlen(homPos_str);
        double homPos = atof(pThisOption);
        setDoubleParam(pC_->EthercatMCHomPos_, homPos);
      } else if (!strncmp(pThisOption, scaleFactor_str, strlen(scaleFactor_str))) {
        pThisOption += strlen(scaleFactor_str);
        drvlocal.scaleFactor = atof(pThisOption);
      }
      pThisOption = pNextOption;
    }
    free(pOptions);
  }
  /* Set the module name to "" if we have FILE/LINE enabled by asyn */
  if (pasynTrace->getTraceInfoMask(pC_->pasynUserController_) & ASYN_TRACEINFO_SOURCE) modNamEMC = "";
  initialPoll();
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

void EthercatMCIndexerAxis::handleDisconnect(asynStatus status)
{
  (void)status;
  if (!drvlocal.dirty.oldStatusDisconnected) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s Communication error(%d)\n", modNamEMC, axisNo_);
  }
  memset(&drvlocal.dirty, 0xFF, sizeof(drvlocal.dirty));
  //drvlocal.MCU_nErrorId = 0;
  setIntegerParam(pC_->motorStatusCommsError_, 1);
  //callParamCallbacksUpdateError();
}

void EthercatMCIndexerAxis::setIndexerTypeCodeOffset(unsigned iTypCode, unsigned iOffset)
{
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%s (%d) iTypCode=0x%x, iOffset=%u\n",
            modNamEMC, axisNo_, iTypCode, iOffset);
  drvlocal.iTypCode = iTypCode;
  drvlocal.iOffset = iOffset;
}


asynStatus EthercatMCIndexerAxis::initialPoll(void)
{
  asynStatus status = asynSuccess;

  if (!drvlocal.dirty.initialPollNeeded)
    return asynSuccess;

#if 1
  return status;
#else
  status = initialPollInternal();
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sinitialPoll(%d) status=%d\n",
            modNamEMC, axisNo_, status);
  if (status == asynSuccess) drvlocal.dirty.initialPollNeeded = 0;
  return status;
#endif
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
       0x4 Setpoint       32 bit float
       0x8 CmdStatusReason 16 bit integer/bitwise
       0xA ParamCmd        16 bit integer/bitwise
       0xE ParamValue      32 bit float or integer
    */
  unsigned paramIfOffset = drvlocal.iOffset + 0xA;
  asynPrint(pC_->pasynUserController_, traceMask,
            "%smove (%d) position=%f relative=%d minVelocity=%f maxVelocity=%f acceleration=%f\n",
            "EthercatMCIndexerAxis", axisNo_,
            position, relative, minVelocity, maxVelocity, acceleration);
  status = pC_->indexerParamWrite(paramIfOffset,
                                  PARAM_IDX_SPEED_FLOAT32, maxVelocity);
  if (status) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%smove (%d) status=%s (%d)\n",
              "EthercatMCIndexerAxis", axisNo_,
              pasynManager->strStatus(status), (int)status);
    return status;
  }
  status = pC_->indexerParamWrite(paramIfOffset,
                                  PARAM_IDX_ACCEL_FLOAT32, acceleration);
  if (status) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%smove (%d) status=%s (%d)\n",
              "EthercatMCIndexerAxis", axisNo_,
              pasynManager->strStatus(status), (int)status);
    return status;
  }

  /* Write the position into offset 4 */
  status = pC_->setPlcMemoryDouble(drvlocal.iOffset + 4,
                                   position, 4 /* lenInPlc */);
  asynPrint(pC_->pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, pC_->outString_, pC_->inString_,
            pasynManager->strStatus(status), (int)status);
  if (status) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%smove (%d) status=%s (%d)\n",
              "EthercatMCIndexerAxis", axisNo_,
              pasynManager->strStatus(status), (int)status);
    return status;
  } else {
    unsigned cmdReason = idxStatusCodeSTART  << 12;
    status = pC_->setPlcMemoryInteger(drvlocal.iOffset + 8,
                                      cmdReason, 2 /* lenInPlc */);
    if (status) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
    asynPrint(pC_->pasynUserController_, traceMask,
              "%sout=%s in=%s status=%s (%d)\n",
              modNamEMC, pC_->outString_, pC_->inString_,
              pasynManager->strStatus(status), (int)status);
  }
  return status;
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
  asynStatus status = asynSuccess;
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
  return asynSuccess;
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
  asynStatus status = asynSuccess;
  if (drvlocal.iTypCode && drvlocal.iOffset) {
    unsigned traceMask = ASYN_TRACE_INFO;
    double actPosition = 0.0;
    {
      size_t lenInPlc = 4;
      unsigned regSize = 2;
      unsigned reg = 0;
      status = pC_->getPlcMemoryDouble(drvlocal.iOffset + reg * regSize,
                                       &actPosition, lenInPlc);
      if (!status) {
        setDoubleParam(pC_->motorPosition_, actPosition);
      } else {
        asynPrint(pC_->pasynUserController_, traceMask,
                  "%sout=%s in=%s status=%s (%d)\n",
                  modNamEMC, pC_->outString_, pC_->inString_,
                  pasynManager->strStatus(status), (int)status);
      }
    }
    {
      unsigned statusReasonAux;
      size_t lenInPlc = 2;
      unsigned reg = 4;
      idxStatusCodeType idxStatusCode;
      bool nowMoving = false;
      drvlocal.hasError = 0;
      status = pC_->getPlcMemoryUint(drvlocal.iOffset + reg * lenInPlc,
                                          &statusReasonAux, lenInPlc);
      if (status) {
        asynPrint(pC_->pasynUserController_, traceMask,
                  "%spoll(%d) status=%d\n",
                  modNamEMC, axisNo_, (int)status);
      }
      if (!status && (statusReasonAux != drvlocal.old_tatusReasonAux)) {
        int hls = 0;
        int lls = 0;
        int powerIsOn = 1; /* Unless powerOff */
        idxStatusCode = (idxStatusCodeType)(statusReasonAux >> 12);
#ifdef EthercatMCStatusCodeString
        setIntegerParam(pC_->EthercatMCStatusCode_, idxStatusCode);
#endif
        asynPrint(pC_->pasynUserController_, traceMask,
                  "%spoll(%d) pos=%f statusReasonAux=0x%x %d (%s)\n",
                  modNamEMC, axisNo_,
                  actPosition,
                  statusReasonAux, idxStatusCode,
                  idxStatusCodeTypeToStr(idxStatusCode));
        switch (idxStatusCode) {
          case idxStatusCodeRESET:
            drvlocal.hasError = 1;
            break;
          case idxStatusCodeIDLE:
            break;
          case idxStatusCodePOWEROFF:
            powerIsOn = 0;
            break;
          case idxStatusCodeWARN:
            break;
          case idxStatusCodeBUSY:
            nowMoving = true;
            break;
          case idxStatusCodeSTOP:
            nowMoving = true;
            break;
          case idxStatusCodeERROR:
          default:
            drvlocal.hasError = 1;
        }
        if ((idxStatusCode == idxStatusCodeWARN) ||
            (idxStatusCode == idxStatusCodeERROR)) {
          hls = statusReasonAux & 0x0800 ? 1 : 0;
          lls = statusReasonAux & 0x0400 ? 1 : 0;
        }
        *moving = nowMoving;
        setIntegerParam(pC_->motorStatusProblem_, drvlocal.hasError);
        setIntegerParam(pC_->motorStatusMoving_, nowMoving);
        setIntegerParam(pC_->motorStatusDone_, !nowMoving);
        setIntegerParam(pC_->motorStatusCommsError_, 0);
        setIntegerParam(pC_->motorStatusLowLimit_, lls);
        setIntegerParam(pC_->motorStatusHighLimit_, hls);
        setIntegerParam(pC_->motorStatusPowerOn_, powerIsOn);

        drvlocal.old_tatusReasonAux = statusReasonAux;
      }
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
