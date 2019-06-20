/*
  FILENAME... EthercatMCAxis.cpp
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>

#include <epicsThread.h>

#include "motor.h"
#include "EthercatMCAxis.h"
#include "EthercatMCController.h"

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

#define NCOMMANDMOVEVEL  1
#define NCOMMANDMOVEREL  2
#define NCOMMANDMOVEABS  3
#define NCOMMANDHOME    10
#define HOMPROC_MANUAL_SETPOS    15

/* The maximum number of polls we wait for the motor
   to "start" (report moving after a new move command */
#define WAITNUMPOLLSBEFOREREADY 3


//
// These are the EthercatMCAxis methods
//

/** Creates a new EthercatMCAxis object.
 * \param[in] pC Pointer to the EthercatMCController to which this axis belongs.
 * \param[in] axisNo Index number of this axis, range 1 to pC->numAxes_. (0 is not used)
 *
 *
 * Initializes register numbers, etc.
 */
EthercatMCAxis::EthercatMCAxis(EthercatMCController *pC, int axisNo,
                     int axisFlags, const char *axisOptionsStr)
  : asynMotorAxis(pC, axisNo),
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
  /* Force a printout of this 3 variables, once running they are  0 or 1 */
  drvlocal.old_st_axis_status.bHomed = -1;
  drvlocal.old_st_axis_status.bLimitBwd = -1;
  drvlocal.old_st_axis_status.bLimitFwd = -1;
  drvlocal.old_eeAxisError = eeAxisErrorIOCcomError;
  drvlocal.axisFlags = axisFlags;

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
  if (axisFlags & AMPLIFIER_ON_FLAG_WHEN_HOMING) {
#ifdef POWERAUTOONOFFMODE2
    setIntegerParam(pC_->motorPowerAutoOnOff_, POWERAUTOONOFFMODE2);
    setDoubleParam(pC_->motorPowerOnDelay_,   6.0);
    setDoubleParam(pC_->motorPowerOffDelay_, -1.0);
#endif
#ifdef motorShowPowerOffString
    setIntegerParam(pC_->motorShowPowerOff_, 1);
#endif
#ifdef  motorNotHomedProblemString
    setIntegerParam(pC_->motorNotHomedProblem_, MOTORNOTHOMEDPROBLEM_ERROR);
#endif

  }
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
    const char * const sFeatures_str = "sFeatures=";

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
        drvlocal.cfgfileStr = strdup(pThisOption);
      } else if (!strncmp(pThisOption, cfgDebug_str, strlen(cfgDebug_str))) {
        pThisOption += strlen(cfgDebug_str);
        drvlocal.cfgDebug_str = strdup(pThisOption);
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
      } else if (!strncmp(pThisOption, sFeatures_str, strlen(sFeatures_str))) {
        pThisOption += strlen(sFeatures_str);
        if (!strcmp(pThisOption, "Gvl")) {
          pC_->features_ |= FEATURE_BITS_GVL;
        }
      }
      pThisOption = pNextOption;
    }
    free(pOptions);
  }
  /* Set the module name to "" if we have FILE/LINE enabled by asyn */
  if (pasynTrace->getTraceInfoMask(pC_->pasynUserController_) & ASYN_TRACEINFO_SOURCE) modNamEMC = "";
  initialPoll();
}


extern "C" int EthercatMCCreateAxis(const char *EthercatMCName, int axisNo,
                               int axisFlags, const char *axisOptionsStr)
{
  EthercatMCController *pC;

  pC = (EthercatMCController*) findAsynPortDriver(EthercatMCName);
  if (!pC)
  {
    printf("Error port %s not found\n", EthercatMCName);
    return asynError;
  }
  pC->lock();
  new EthercatMCAxis(pC, axisNo, axisFlags, axisOptionsStr);
  pC->unlock();
  return asynSuccess;
}


asynStatus EthercatMCAxis::readBackSoftLimits(void)
{
  asynStatus status;
  int nvals;
  int axisID = getMotionAxisID();
  int enabledHigh = 0, enabledLow = 0;
  double fValueHigh = 0.0, fValueLow  = 0.0;
  double scaleFactor = drvlocal.scaleFactor;

  snprintf(pC_->outString_, sizeof(pC_->outString_),
           "ADSPORT=501/.ADR.16#%X,16#%X,2,2?;ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,2,2?;ADSPORT=501/.ADR.16#%X,16#%X,8,5?",
           0x5000 + axisID, 0xC,
           0x5000 + axisID, 0xE,
           0x5000 + axisID, 0xB,
           0x5000 + axisID, 0xD);

  status = pC_->writeReadOnErrorDisconnect();
  if (status)
    return status;
  nvals = sscanf(pC_->inString_, "%d;%lf;%d;%lf",
                 &enabledHigh, &fValueHigh, &enabledLow, &fValueLow);
  if (nvals != 4) {
     asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
     "%snvals=%d command=\"%s\" response=\"%s\"\n",
             modNamEMC, nvals, pC_->outString_, pC_->inString_);
    enabledHigh = enabledLow = 0;
    fValueHigh = fValueLow = 0.0;
    return asynError;
  }
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_FLOW,
            "%sout=%s in=%s CHLM_En=%d CHLM=%f CLLM_En=%d CLLM=%f\n",
            modNamEMC, pC_->outString_, pC_->inString_,
            enabledHigh, fValueHigh, enabledLow, fValueLow);
  /* EthercatMCCHLMXX are info(asyn:READBACK,"1"),
     so we must use pC_->setXXX(axisNo_..)  here */
  pC_->setIntegerParam(axisNo_, pC_->EthercatMCCfgDHLM_En_, enabledHigh);
  pC_->setDoubleParam(axisNo_, pC_->EthercatMCCfgDHLM_, fValueHigh);
  pC_->setIntegerParam(axisNo_, pC_->EthercatMCCfgDLLM_En_, enabledLow);
  pC_->setDoubleParam(axisNo_, pC_->EthercatMCCfgDLLM_, fValueLow);

  if (scaleFactor) {
    pC_->udateMotorLimitsRO(axisNo_, enabledHigh && enabledLow,
                            fValueHigh / scaleFactor, fValueLow / scaleFactor);
  }
  return status;
}

asynStatus EthercatMCAxis::readBackHoming(void)
{
  asynStatus status;
  int    homProc = 0;
  double homPos  = 0.0;

  /* Don't read it, when the driver has been configured with HomProc= */
  status = pC_->getIntegerParam(axisNo_, pC_->EthercatMCHomProc_,&homProc);
  if (status == asynSuccess) return status;

  status = getValueFromAxis("_EPICS_HOMPROC", &homProc);
  if (!status) setIntegerParam(pC_->EthercatMCHomProc_, homProc);
  status = getValueFromAxis("_EPICS_HOMPOS", &homPos);
  if (status) {
    /* fall back */
    status = getSAFValueFromAxisPrint(0x5000, 0x103, "homPos", &homPos);
  }
  if (!status) setDoubleParam(pC_->EthercatMCHomPos_, homPos);
  return asynSuccess;
}

asynStatus EthercatMCAxis::readScaling(int axisID)
{
  int nvals;
  asynStatus status;
  double srev = 0, urev = 0, old_srev = 0, old_urev = 0;

  double scaleFactor = drvlocal.scaleFactor;

  if (!scaleFactor) return asynError;
  snprintf(pC_->outString_, sizeof(pC_->outString_),
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;",
           0x5000 + axisID, 0x24,  // SREV
           0x5000 + axisID, 0x23   // UREV
           );
  status = pC_->writeReadOnErrorDisconnect();
  if (status) return status;
  nvals = sscanf(pC_->inString_, "%lf;%lf",
                 &srev, &urev);

  if (nvals != 2) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%snvals=%d\n", modNamEMC, nvals);
    return asynError;
  }

  pC_->getDoubleParam(axisNo_, pC_->EthercatMCCfgSREV_RB_, &old_srev);
  pC_->getDoubleParam(axisNo_, pC_->EthercatMCCfgUREV_RB_, &old_urev);
  if (srev != old_srev || urev != old_urev) {
    setDoubleParam(pC_->EthercatMCCfgSREV_RB_, srev);
    setDoubleParam(pC_->EthercatMCCfgUREV_RB_, urev);
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssrev=%f urev=%f\n",
              modNamEMC, srev, urev);
  }
  return asynSuccess;
}

asynStatus EthercatMCAxis::readMonitoring(int axisID)
{
  int nvals;
  asynStatus status;
  double old_rdbd = 0, rdbd, old_rdbd_tim = 0, rdbd_tim, poslag = -1, poslag_tim = -1;
  int old_rdbd_en = 0, rdbd_en, poslag_en = 0;
  double scaleFactor = drvlocal.scaleFactor;

  if (!scaleFactor) return asynError;
  snprintf(pC_->outString_, sizeof(pC_->outString_),
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,2,2?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,2,2?",
           0x4000 + axisID, 0x16,  // RDBD_RB
           0x4000 + axisID, 0x17,  // RDBD_Tim
           0x4000 + axisID, 0x15,  // RDND_En
           0x6000 + axisID, 0x12,  // PosLag
           0x6000 + axisID, 0x13,  // PosLag_Tim
           0x6000 + axisID, 0x10); // Poslag_En
  status = pC_->writeReadOnErrorDisconnect();
  if (status) return status;
  nvals = sscanf(pC_->inString_, "%lf;%lf;%d;%lf;%lf;%d",
                 &rdbd, &rdbd_tim, &rdbd_en, &poslag, &poslag_tim, &poslag_en
                 );
  if ((nvals != 6) && (nvals != 3)) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sreadMonitoring(%d) nvals=%d out=%s in=%s \n",
              modNamEMC, axisNo_, nvals, pC_->outString_, pC_->inString_);
    return asynError;
  }
  pC_->getDoubleParam(axisNo_,  pC_->EthercatMCCfgRDBD_RB_,     &old_rdbd);
  pC_->getDoubleParam(axisNo_,  pC_->EthercatMCCfgRDBD_Tim_RB_, &old_rdbd_tim);
  pC_->getIntegerParam(axisNo_, pC_->EthercatMCCfgRDBD_En_RB_,  &old_rdbd_en);
  if (rdbd != old_rdbd || old_rdbd_tim != rdbd_tim || old_rdbd_en != rdbd_en) {
    /* RDBD is really important, print it on change */
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%s (%d) rdbd=%f, rdbd_tim=%f rdbd_en=%d\n",
              modNamEMC, axisNo_, rdbd, rdbd_tim, rdbd_en);
  }
  /* poslag (position lag) is much less important, print via "FLOW" */
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_FLOW,
            "%s (%d) poslag=%f poslag_tim=%f poslag_en=%d\n",
            modNamEMC, axisNo_, poslag, poslag_tim, poslag_en);

  /* Must set SPDB before setting RDBD.
     See motorRecord.cc enforceMinRetryDeadband() */
  setDoubleParam(pC_->EthercatMCCfgSPDB_RB_, rdbd_en ? rdbd / scaleFactor : 0.0);

  setDoubleParam(pC_->EthercatMCCfgRDBD_RB_, rdbd);
  setDoubleParam(pC_->EthercatMCCfgRDBD_Tim_RB_, rdbd_tim);
  setIntegerParam(pC_->EthercatMCCfgRDBD_En_RB_, rdbd_en);
  /* Either the monitoring is off or 0.0 by mistake, set an error */
  drvlocal.illegalInTargetWindow = (!rdbd_en || !rdbd);

  if (nvals == 6) {
    setDoubleParam(pC_->EthercatMCCfgPOSLAG_RB_, poslag);
    setDoubleParam(pC_->EthercatMCCfgPOSLAG_Tim_RB_, poslag_tim);
    setIntegerParam(pC_->EthercatMCCfgPOSLAG_En_RB_, poslag_en);
  }
  return asynSuccess;
}


asynStatus EthercatMCAxis::readBackVelocities(int axisID)
{
  asynStatus status;
  int nvals;
  double scaleFactor = drvlocal.scaleFactor;
  double velo, vmax, jvel, accs;
  snprintf(pC_->outString_, sizeof(pC_->outString_),
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;"
           "ADSPORT=501/.ADR.16#%X,16#%X,8,5?;",
           0x4000 + axisID, 0x9,   // VELO"
           0x4000 + axisID, 0x27,  // VMAX"
           0x4000 + axisID, 0x8,   // JVEL
           0x4000 + axisID, 0x101  // ACCS"
           );
  status = pC_->writeReadOnErrorDisconnect();
  if (status) return status;
  nvals = sscanf(pC_->inString_, "%lf;%lf;%lf;%lf",
                 &velo, &vmax, &jvel, &accs);
  if (nvals != 4) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%snvals=%d command=\"%s\" response=\"%s\"\n",
              modNamEMC, nvals, pC_->outString_, pC_->inString_);
    return asynError;
  }
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_FLOW,
            "%svelo=%f vmax=%f jvel=%f accs=%f\n",
            modNamEMC, velo, vmax, jvel, accs);
  if (velo > 0.0) {
    pC_->setDoubleParam(axisNo_, pC_->EthercatMCCfgVELO_, velo / scaleFactor);
  }
  if (vmax > 0.0) {
    pC_->setDoubleParam(axisNo_, pC_->EthercatMCCfgVMAX_, vmax / scaleFactor);
  }
  if (jvel > 0.0) {
    pC_->setDoubleParam(axisNo_, pC_->EthercatMCCfgJVEL_, jvel / scaleFactor);
  }
  if (accs > 0.0) {
    pC_->setDoubleParam(axisNo_, pC_->EthercatMCCfgACCS_, accs / scaleFactor);
  }
  return asynSuccess;
}

asynStatus EthercatMCAxis::readBackEncoders(int axisID)
{
  /* https://infosys.beckhoff.com/english.php?content=../content/1033/tcadsdevicenc2/index.html&id= */
  asynStatus status;
  uint32_t numEncoders = 0;
  int nvals;

  snprintf(pC_->outString_, sizeof(pC_->outString_),
           "ADSPORT=501/.ADR.16#%X,16#%X,8,19?;",
           0x4000 + axisID, 0x57
           );
  status = pC_->writeReadOnErrorDisconnect();
  if (status) return status;
  nvals = sscanf(pC_->inString_, "%u",
                 &numEncoders);
  if (nvals != 1) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%snvals=%d command=\"%s\" response=\"%s\"\n",
              modNamEMC, nvals, pC_->outString_, pC_->inString_);
    return asynError;
  }
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sreadBackEncoders(%d) numEncoders=%u\n",
            modNamEMC, axisNo_, numEncoders);
  return asynSuccess;
}

asynStatus EthercatMCAxis::initialPoll(void)
{
  asynStatus status;

  if (!drvlocal.dirty.initialPollNeeded)
    return asynSuccess;

  status = initialPollInternal();
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sinitialPoll(%d) status=%d\n",
            modNamEMC, axisNo_, status);
  if (status == asynSuccess) drvlocal.dirty.initialPollNeeded = 0;
  return status;
}


asynStatus EthercatMCAxis::readBackAllConfig(int axisID)
{
  asynStatus status = asynSuccess;
  /* for ECMC homing is configured from EPICS, do NOT do the readback */
  if (!(pC_->features_ & FEATURE_BITS_ECMC)) {
    if (!drvlocal.scaleFactor) status = asynError;
    if (status == asynSuccess) status = readBackHoming();
  }
  if (status == asynSuccess) status = readScaling(axisID);
  if (status == asynSuccess) status = readMonitoring(axisID);
  if (status == asynSuccess) status = readBackSoftLimits();
  if (status == asynSuccess) status = readBackVelocities(axisID);
  if (status == asynSuccess) status = readBackEncoders(axisID);
  return status;
}


/** Connection status is changed, the dirty bits must be set and
 *  the values in the controller must be updated
 * \param[in] AsynStatus status
 *
 * Sets the dirty bits
 */
asynStatus EthercatMCAxis::initialPollInternal(void)
{
  asynStatus status = asynSuccess;

  /*  Check for Axis ID */
  int axisID = getMotionAxisID();
  switch (axisID) {
    case -2:
      updateMsgTxtFromDriver("No AxisID");
      return asynSuccess;
    case -1:
      setIntegerParam(pC_->motorStatusCommsError_, 1);
      return asynError;
    case 0:
      return asynSuccess;
    default:
      if (axisID != axisNo_) {
        updateMsgTxtFromDriver("ConfigError AxisID");
        return asynError;
      }
  }
  status = readConfigFile();
  if (status) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s(%d) readConfigFile() failed\n",
               modNamEMC, axisNo_);
    updateMsgTxtFromDriver("ConfigError Config File");
    return status;
  }
  if (drvlocal.axisFlags & AMPLIFIER_ON_FLAG_CREATE_AXIS) {
    /* Enable the amplifier when the axis is created,
       but wait until we have a connection to the controller.
       After we lost the connection, Re-enable the amplifier
       See AMPLIFIER_ON_FLAG */
    status = enableAmplifier(1);
  }
  if (status == asynSuccess) status = readBackAllConfig(axisID);
  if (status == asynSuccess && drvlocal.dirty.oldStatusDisconnected) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sconnected(%d)\n", modNamEMC, axisNo_);
    drvlocal.dirty.oldStatusDisconnected = 0;
  }
  return status;
}

/** Reports on status of the axis
 * \param[in] fp The file pointer on which report information will be written
 * \param[in] level The level of report detail desired
 *
 * After printing device-specific information calls asynMotorAxis::report()
 */
void EthercatMCAxis::report(FILE *fp, int level)
{
  if (level > 0) {
    fprintf(fp, "  axis %d\n", axisNo_);
  }

  // Call the base class method
  asynMotorAxis::report(fp, level);
}


/** Set velocity and acceleration for the axis
 * \param[in] maxVelocity, mm/sec
 * \param[in] acceleration ???
 *
 */
asynStatus EthercatMCAxis::sendVelocityAndAccelExecute(double maxVeloEGU, double accEGU)
{
  asynStatus status;
  /* We don't use minVelocity */
  status = setValuesOnAxis("fAcceleration", accEGU,
                           "fDeceleration", accEGU);
  if (status) return status;
  status = setValueOnAxis("fVelocity", maxVeloEGU);
  if (status == asynSuccess) status = setValueOnAxis("bExecute", 1);
#ifndef motorWaitPollsBeforeReadyString
  drvlocal.waitNumPollsBeforeReady += WAITNUMPOLLSBEFOREREADY;
#endif
  return status;
}

/** Move the motor to an absolute location or by a relative amount.
  * \param[in] posEGU  The absolute position to move to (if relative=0) or the relative distance to move
  * by (if relative=1). Units=steps.
  * \param[in] relative  Flag indicating relative move (1) or absolute move (0).
  * \param[in] maxVeloEGU The maximum velocity, often called the slew velocity. Units=EGU/sec.
  * \param[in] accEGU The acceleration value. Units=EGU/sec/sec. */
asynStatus EthercatMCAxis::mov2(double posEGU, int nCommand, double maxVeloEGU, double accEGU)
{
  if (accEGU) {
    snprintf(pC_->outString_, sizeof(pC_->outString_),
             "%sMain.M%d.bExecute=0;"
             "%sMain.M%d.nCommand=%d;"
             "%sMain.M%d.nCmdData=0;"
             "%sMain.M%d.fPosition=%f;"
             "%sMain.M%d.fAcceleration=%f;"
             "%sMain.M%d.fDeceleration=%f;"
             "%sMain.M%d.fVelocity=%f;"
             "%sMain.M%d.bExecute=1",
             drvlocal.adsport_str, axisNo_,
             drvlocal.adsport_str, axisNo_, nCommand,
             drvlocal.adsport_str, axisNo_,
             drvlocal.adsport_str, axisNo_, posEGU,
             drvlocal.adsport_str, axisNo_, accEGU,
             drvlocal.adsport_str, axisNo_, accEGU,
             drvlocal.adsport_str, axisNo_, maxVeloEGU,
             drvlocal.adsport_str, axisNo_);
  } else {
    snprintf(pC_->outString_, sizeof(pC_->outString_),
             "%sMain.M%d.bExecute=0;"
             "%sMain.M%d.nCommand=%d;"
             "%sMain.M%d.nCmdData=0;"
             "%sMain.M%d.fPosition=%f;"
             "%sMain.M%d.fVelocity=%f;"
             "%sMain.M%d.bExecute=1",
             drvlocal.adsport_str, axisNo_,
             drvlocal.adsport_str, axisNo_, nCommand,
             drvlocal.adsport_str, axisNo_,
             drvlocal.adsport_str, axisNo_, posEGU,
             drvlocal.adsport_str, axisNo_, maxVeloEGU,
             drvlocal.adsport_str, axisNo_);
  }
#ifndef motorWaitPollsBeforeReadyString
  drvlocal.waitNumPollsBeforeReady += WAITNUMPOLLSBEFOREREADY;
#endif
  return pC_->writeReadACK(ASYN_TRACE_INFO);
}

/** Move the axis to a position, either absolute or relative
 * \param[in] position in steps
 * \param[in] relative (0=absolute, otherwise relative)
 * \param[in] minimum velocity, steps/sec
 * \param[in] maximum velocity, steps/sec
 * \param[in] acceleration,  steps/sec/sec
 *
 */
asynStatus EthercatMCAxis::move(double position, int relative, double minVelocity, double maxVelocity, double acceleration)
{
  asynStatus status = asynSuccess;

  drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
  /* Do range check */
  if (!drvlocal.scaleFactor) {
    drvlocal.eeAxisWarning = eeAxisWarningCfgZero;
    return asynSuccess;
  } else if (!maxVelocity) {
    drvlocal.eeAxisWarning = eeAxisWarningVeloZero;
    return asynSuccess;
  }

#if MAX_CONTROLLER_STRING_SIZE > 350
  return mov2(position * drvlocal.scaleFactor,
              relative ? NCOMMANDMOVEREL : NCOMMANDMOVEABS,
              maxVelocity * drvlocal.scaleFactor,
              acceleration * drvlocal.scaleFactor);
#else
  int nCommand = relative ? NCOMMANDMOVEREL : NCOMMANDMOVEABS;
  if (status == asynSuccess) status = stopAxisInternal(__FUNCTION__, 0);
  if (status == asynSuccess) status = setValueOnAxis("nCommand", nCommand);
  if (status == asynSuccess) status = setValueOnAxis("nCmdData", 0);
  if (status == asynSuccess) status = setValueOnAxis("fPosition", position * drvlocal.scaleFactor);
  if (status == asynSuccess) status = sendVelocityAndAccelExecute(maxVelocity * drvlocal.scaleFactor,
                                                                  acceleration * drvlocal.scaleFactor);
#endif
  return status;
}


/** Home the motor, search the home position
 * \param[in] minimum velocity, mm/sec
 * \param[in] maximum velocity, mm/sec
 * \param[in] acceleration, seconds to maximum velocity
 * \param[in] forwards (0=backwards, otherwise forwards)
 *
 */
asynStatus EthercatMCAxis::home(double minVelocity, double maxVelocity, double acceleration, int forwards)
{
  asynStatus status = asynSuccess;
  int nCommand = NCOMMANDHOME;

  int homProc = -1;
  double homPos = 0.0;

  drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
  /* The homPos may be undefined, then use 0.0 */
  (void)pC_->getDoubleParam(axisNo_, pC_->EthercatMCHomPos_, &homPos);
  status = pC_->getIntegerParam(axisNo_, pC_->EthercatMCHomProc_,&homProc);
  if (homProc == HOMPROC_MANUAL_SETPOS)
    return asynError;
  /* The controller will do the home search, and change its internal
     raw value to what we specified in fPosition. */
  if (pC_->features_ & FEATURE_BITS_ECMC) {
    double velToHom;
    double velFrmHom;
    double accHom;
    if (!status) status = stopAxisInternal(__FUNCTION__, 0);
    if (!status) status = setValueOnAxis("fHomePosition", homPos);
    if (!status) status = pC_->getDoubleParam(axisNo_,
                                              pC_->EthercatMCVelToHom_,
                                              &velToHom);
    if (!status) status = pC_->getDoubleParam(axisNo_,
                                              pC_->EthercatMCVelFrmHom_,
                                              &velFrmHom);
    if (!status) status = pC_->getDoubleParam(axisNo_,
                                              pC_->EthercatMCAccHom_,
                                              &accHom);
    if (!status) status = setSAFValueOnAxis(0x4000, 0x6,
                                            velToHom);
    if (!status) status = setSAFValueOnAxis(0x4000, 0x7,
                                            velFrmHom);
    if (!status)  status = setValuesOnAxis("fAcceleration", accHom,
                                           "fDeceleration", accHom);
    if (!status) status = setValueOnAxis("nCommand", nCommand );
    if (!status) status = setValueOnAxis("nCmdData", homProc);
    if (!status) status = setValueOnAxis("bExecute", 1);
  } else {
    snprintf(pC_->outString_, sizeof(pC_->outString_),
             "%sMain.M%d.bExecute=0;"
             "%sMain.M%d.nCommand=%d;"
             "%sMain.M%d.nCmdData=%d;"
             "%sMain.M%d.fHomePosition=%f;"
             "%sMain.M%d.bExecute=1",
             drvlocal.adsport_str, axisNo_,
             drvlocal.adsport_str, axisNo_, nCommand,
             drvlocal.adsport_str, axisNo_, homProc,
             drvlocal.adsport_str, axisNo_, homPos,
             drvlocal.adsport_str, axisNo_);
    return pC_->writeReadACK(ASYN_TRACE_INFO);
  }
#ifndef motorWaitPollsBeforeReadyString
  drvlocal.waitNumPollsBeforeReady += WAITNUMPOLLSBEFOREREADY;
#endif
  return status;
}


/** jog the the motor, search the home position
 * \param[in] minimum velocity, mm/sec (not used)
 * \param[in] maximum velocity, mm/sec (positive or negative)
 * \param[in] acceleration, seconds to maximum velocity
 *
 */
asynStatus EthercatMCAxis::moveVelocity(double minVelocity, double maxVelocity, double acceleration)
{
  drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
  /* Do range check */
  if (!drvlocal.scaleFactor) {
    drvlocal.eeAxisWarning = eeAxisWarningCfgZero;
    return asynSuccess;
  } else if (!maxVelocity) {
    drvlocal.eeAxisWarning = eeAxisWarningVeloZero;
    return asynSuccess;
  }

#if MAX_CONTROLLER_STRING_SIZE > 350
  {
    double maxVeloEGU = maxVelocity * drvlocal.scaleFactor;
    double acc_in_EGU_sec2 = 0.0;
    if (acceleration > 0.0001) {
      double acc_in_seconds = maxVelocity / acceleration;
      acc_in_EGU_sec2 = maxVeloEGU / acc_in_seconds;
    }
    if (acc_in_EGU_sec2  < 0) acc_in_EGU_sec2 = 0 - acc_in_EGU_sec2 ;
    return mov2(0, NCOMMANDMOVEVEL, maxVeloEGU, acc_in_EGU_sec2);
  }
#else
  asynStatus status = asynSuccess;

  if (status == asynSuccess) status = stopAxisInternal(__FUNCTION__, 0);
  if (status == asynSuccess) setValueOnAxis("nCommand", NCOMMANDMOVEVEL);
  if (status == asynSuccess) status = setValueOnAxis("nCmdData", 0);
  if (status == asynSuccess) status = sendVelocityAndAccelExecute(maxVelocity * drvlocal.scaleFactor,
                                                                  acceleration * drvlocal.scaleFactor);

  return status;
#endif
}



/**
 * See asynMotorAxis::setPosition
 */
asynStatus EthercatMCAxis::setPosition(double value)
{
  asynStatus status = asynSuccess;
  int nCommand = NCOMMANDHOME;
  int homProc = 0;
  double homPos = value;

  status = pC_->getIntegerParam(axisNo_,
                                pC_->EthercatMCHomProc_,
                                &homProc);
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%ssetPosition(%d  homProc=%d position=%g egu=%g\n",
            modNamEMC, axisNo_,  homProc, value, value * drvlocal.scaleFactor );

  if (homProc != HOMPROC_MANUAL_SETPOS)
    return asynError;
  if (status == asynSuccess) status = stopAxisInternal(__FUNCTION__, 0);
  if (status == asynSuccess) status = setValueOnAxis("fHomePosition", homPos);
  if (status == asynSuccess) status = setValueOnAxis("nCommand", nCommand );
  if (status == asynSuccess) status = setValueOnAxis("nCmdData", homProc);
  if (status == asynSuccess) status = setValueOnAxis("bExecute", 1);

  return status;
}

asynStatus EthercatMCAxis::resetAxis(void)
{
  asynStatus status = asynSuccess;
  int EthercatMCErr;
  bool moving;
  /* Reset command error, if any */
  drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
  drvlocal.cmdErrorMessage[0] = 0;
  status = pC_->getIntegerParam(axisNo_, pC_->EthercatMCErr_, &EthercatMCErr);
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sresetAxis(%d status=%d EthercatMCErr)=%d\n",
             modNamEMC, axisNo_, (int)status, EthercatMCErr);

  if (EthercatMCErr) {
    /* Soft reset of the axis */
    status = setValueOnAxis("bExecute", 0);
    if (status) goto resetAxisReturn;
    status = setValueOnAxisVerify("bReset", "bReset", 1, 20);
    if (status) goto resetAxisReturn;
    epicsThreadSleep(.1);
    status = setValueOnAxisVerify("bReset", "bReset", 0, 20);
  }
  resetAxisReturn:
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sresetAxis(%d) status=%s (%d)\n",
             modNamEMC, axisNo_, EthercatMCstrStatus(status), (int)status);
  /* do a poll */
  poll(&moving);
  return status;
}

bool EthercatMCAxis::pollPowerIsOn(void)
{
  int ret = 0;
  asynStatus status = getValueFromAxis(".bEnabled", &ret);
  if (!status && ret)
    return true;
  else
    return false;
}

/** Enable the amplifier on an axis
 *
 */
asynStatus EthercatMCAxis::enableAmplifier(int on)
{
  asynStatus status = asynSuccess;
  unsigned counter = 10;
  bool moving;
  int ret;
  const char *enableEnabledReadback = "bEnabled";

#ifdef POWERAUTOONOFFMODE2
  {
    int autoPower;
    pC_->getIntegerParam(axisNo_, pC_->motorPowerAutoOnOff_, &autoPower);
    if (autoPower) {
      /* The record/driver will check for enabled - don't do that here */
      enableEnabledReadback = "bEnable";
    }
  }
#endif
  on = on ? 1 : 0; /* either 0 or 1 */
  status = getValueFromAxis(".bEnabled", &ret);
  /* Either it went wrong OR the amplifier IS as it should be */
  if (status || (ret == on)) return status;
  if (!on) {
    /* Amplifier is on and should be turned off.
       Stop the axis by setting bEnable to 0 */
    status = stopAxisInternal(__FUNCTION__, 0);
    if (status) return status;
  }
  status = setValueOnAxis("bEnable", on);
  if (status || !on) return status; /* this went wrong OR it should be turned off */
  while (counter) {
    epicsThreadSleep(.1);
    snprintf(pC_->outString_, sizeof(pC_->outString_),
             "%sMain.M%d.%s?;%sMain.M%d.%s?",
             drvlocal.adsport_str, axisNo_, "bBusy",
             drvlocal.adsport_str, axisNo_, enableEnabledReadback);
    status = pC_->writeReadOnErrorDisconnect();
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%sout=%s in=%s status=%s (%d)\n",
               modNamEMC, pC_->outString_, pC_->inString_,
              EthercatMCstrStatus(status), (int)status);
    if (status) return status;
    if (!strcmp("0;1", pC_->inString_)) {
      /* bBusy == 0; bEnable(d) == 1 */
      goto enableAmplifierPollAndReturn;
    } else if (!strcmp("1;1", pC_->inString_)) {
      /* bBusy=1 is OK */
      goto enableAmplifierPollAndReturn;
    }
    counter--;
  }
  /* if we come here, it went wrong */
  if (!drvlocal.cmdErrorMessage[0]) {
    snprintf(drvlocal.cmdErrorMessage, sizeof(drvlocal.cmdErrorMessage)-1,
             "E: enableAmplifier(%d) failed. out=%s in=%s\n",
             axisNo_, pC_->outString_, pC_->inString_);
    /* The poller co-ordinates the writing into the parameter library */
  }
enableAmplifierPollAndReturn:
  poll(&moving);
  return status;

}

/** Stop the axis
 *
 */
asynStatus EthercatMCAxis::stopAxisInternal(const char *function_name, double acceleration)
{
  asynStatus status = asynError;
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%sstopAxisInternal(%d) (%s)\n", modNamEMC, axisNo_, function_name);
  status = setValueOnAxisVerify("bExecute", "bExecute", 0, 1);
  return status;
}

/** Stop the axis, called by motor Record
 *
 */
asynStatus EthercatMCAxis::stop(double acceleration )
{
  drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
  return stopAxisInternal(__FUNCTION__, acceleration);
}

void EthercatMCAxis::callParamCallbacksUpdateError()
{
  const char *msgTxtFromDriver = NULL;
  int EPICS_nErrorId = drvlocal.MCU_nErrorId;
  drvlocal.eeAxisError = eeAxisErrorNoError;
  if (drvlocal.supported.statusVer == -1) {
    drvlocal.eeAxisError = eeAxisErrorNotFound;
    msgTxtFromDriver = "Not found";
  } else if (EPICS_nErrorId) {
    /* Error from MCU */
    drvlocal.eeAxisError = eeAxisErrorMCUError;
    msgTxtFromDriver = &drvlocal.sErrorMessage[0];
  } else if (drvlocal.dirty.sErrorMessage) {
    /* print error below */
    drvlocal.eeAxisError = eeAxisErrorIOCcomError;
  } else if (drvlocal.cmdErrorMessage[0]) {
    drvlocal.eeAxisError = eeAxisErrorCmdError;
    msgTxtFromDriver = &drvlocal.cmdErrorMessage[0];
  } else if (!drvlocal.homed &&
             (drvlocal.nCommandActive != NCOMMANDHOME)) {
    drvlocal.eeAxisError = eeAxisErrorNotHomed;
  } else if (drvlocal.illegalInTargetWindow) {
    drvlocal.eeAxisError = eeAxisIllegalInTargetWindow;
    msgTxtFromDriver = "E: InTargetPosWin";
  }
  if (drvlocal.eeAxisError != drvlocal.old_eeAxisError ||
      drvlocal.eeAxisWarning != drvlocal.old_eeAxisWarning ||
      drvlocal.old_EPICS_nErrorId != EPICS_nErrorId ||
      drvlocal.old_nCommandActive != drvlocal.nCommandActive) {

    if (!msgTxtFromDriver && drvlocal.eeAxisWarning) {
       /* No error to show yet */
      switch(drvlocal.eeAxisWarning) {
      case eeAxisWarningCfgZero:
        msgTxtFromDriver = "E: scaleFactor is 0.0";
        break;
      case eeAxisWarningVeloZero:
        msgTxtFromDriver = "E: velo is 0.0";
        break;
      case eeAxisWarningSpeedLimit:
        msgTxtFromDriver = "I: Speed Limit";
        break;
      case eeAxisWarningNoWarning:
        break;
      }
    }
    /* No warning to show yet */
    if (!msgTxtFromDriver) {
      switch(drvlocal.nCommandActive) {
#ifdef motorLatestCommandString
      case NCOMMANDMOVEVEL:
        setIntegerParam(pC_->motorLatestCommand_, LATEST_COMMAND_MOVE_VEL);
        break;
      case NCOMMANDMOVEREL:
        setIntegerParam(pC_->motorLatestCommand_, LATEST_COMMAND_MOVE_REL);
        break;
      case NCOMMANDMOVEABS:
        setIntegerParam(pC_->motorLatestCommand_, LATEST_COMMAND_MOVE_ABS);
        break;
      case NCOMMANDHOME:
        setIntegerParam(pC_->motorLatestCommand_, LATEST_COMMAND_HOMING);
        break;
#endif
      case 0:
        break;
      default:
        msgTxtFromDriver = "I: Moving";
      }
    }
    /* End of error/warning text messages */

    /* Axis has a problem: Report to motor record */
    /*
     * Site note: Some versions of the motor module needed and
     *  #ifdef motorFlagsNoStopProblemString
     * here. Today these versions are history, and the
     * motorFlagsNoStopProblemString is no longer defined in the
     * motor module. So we need to remove the #ifdef here.
     */
    setIntegerParam(pC_->motorStatusProblem_,
                    drvlocal.eeAxisError != eeAxisErrorNoError);
    /* MCU has a problem: set the red light in CSS */
    setIntegerParam(pC_->EthercatMCErr_,
                    drvlocal.eeAxisError == eeAxisErrorMCUError);
    setIntegerParam(pC_->EthercatMCErrId_, EPICS_nErrorId);
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spoll(%d) callParamCallbacksUpdateError"
              " Error=%d old=%d ErrID=0x%x old=0x%x Warn=%d nCmd=%d old=%d txt=%s\n",
              modNamEMC, axisNo_, drvlocal.eeAxisError, drvlocal.old_eeAxisError,
              EPICS_nErrorId, drvlocal.old_EPICS_nErrorId,
              drvlocal.eeAxisWarning,
              drvlocal.nCommandActive, drvlocal.old_nCommandActive,
              msgTxtFromDriver ? msgTxtFromDriver : "NULL");

    if (!drvlocal.cfgDebug_str) {
      updateMsgTxtFromDriver(msgTxtFromDriver);
    }
    drvlocal.old_eeAxisError = drvlocal.eeAxisError;
    drvlocal.old_eeAxisWarning = drvlocal.eeAxisWarning;
    drvlocal.old_EPICS_nErrorId = EPICS_nErrorId;
    drvlocal.old_nCommandActive = drvlocal.nCommandActive;
  }

  callParamCallbacks();
}


asynStatus EthercatMCAxis::pollAll(bool *moving, st_axis_status_type *pst_axis_status)
{
  asynStatus comStatus;

  int motor_axis_no = 0;
  int nvals = 0;
  const char * const Main_dot_str = "Main.";
  const size_t       Main_dot_len = strlen(Main_dot_str);
  struct {
    double velocitySetpoint;
    double fDecceleration;
    int cycleCounter;
    unsigned int EtherCATtime_low32;
    unsigned int EtherCATtime_high32;
    int command;
    int cmdData;
    int reset;
    int moving;
    int stall;
  } notUsed;
  if (drvlocal.dirty.initialPollNeeded) {
    comStatus = initialPoll();
    if (comStatus) return comStatus;
  }

  if (pC_->features_ & FEATURE_BITS_V2) {
    /* V2 is supported, use it. */
    snprintf(pC_->outString_, sizeof(pC_->outString_),
            "%sMain.M%d.stAxisStatusV2?", drvlocal.adsport_str, axisNo_);
    comStatus = pC_->writeReadOnErrorDisconnect();
    if (!strncasecmp(pC_->inString_,  Main_dot_str, Main_dot_len)) {
      nvals = sscanf(&pC_->inString_[Main_dot_len],
                     "M%d.stAxisStatusV2="
                     "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                     &motor_axis_no,
                     &pst_axis_status->fPosition,
                     &pst_axis_status->fActPosition,
                     &pst_axis_status->encoderRaw,          /* Send as uint64; parsed as double ! */
                     &notUsed.velocitySetpoint,
                     &pst_axis_status->fActVelocity,
                     &pst_axis_status->fAcceleration,
                     &notUsed.fDecceleration,
                     &notUsed.cycleCounter,
                     &notUsed.EtherCATtime_low32,
                     &notUsed.EtherCATtime_high32,
                     &pst_axis_status->bEnable,
                     &pst_axis_status->bEnabled,
                     &pst_axis_status->bExecute,
                     &notUsed.command,
                     &notUsed.cmdData,
                     &pst_axis_status->bLimitBwd,
                     &pst_axis_status->bLimitFwd,
                     &pst_axis_status->bHomeSensor,
                     &pst_axis_status->bError,
                     &pst_axis_status->nErrorId,
                     &notUsed.reset,
                     &pst_axis_status->bHomed,
                     &pst_axis_status->bBusy,
                     &pst_axis_status->atTarget,
                     &notUsed.moving,
                     &notUsed.stall);
    }
    if (nvals == 27) {
      pst_axis_status->mvnNRdyNex = pst_axis_status->bBusy || !pst_axis_status->atTarget;
    }
  } else if (pC_->features_ & FEATURE_BITS_V1) {
    /* Read the complete Axis status */
    snprintf(pC_->outString_, sizeof(pC_->outString_),
            "%sMain.M%d.stAxisStatus?", drvlocal.adsport_str, axisNo_);
    comStatus = pC_->writeReadOnErrorDisconnect();
    if (comStatus) return comStatus;
    if (!strncasecmp(pC_->inString_,  Main_dot_str, Main_dot_len)) {
      nvals = sscanf(&pC_->inString_[Main_dot_len],
                     "M%d.stAxisStatus="
                     "%d,%d,%d,%u,%u,%lf,%lf,%lf,%lf,%d,"
                     "%d,%d,%d,%lf,%d,%d,%d,%u,%lf,%lf,%lf,%d,%d",
                     &motor_axis_no,
                     &pst_axis_status->bEnable,        /*  1 */
                     &pst_axis_status->bReset,         /*  2 */
                     &pst_axis_status->bExecute,       /*  3 */
                     &pst_axis_status->nCommand,       /*  4 */
                     &pst_axis_status->nCmdData,       /*  5 */
                     &pst_axis_status->fVelocity,      /*  6 */
                     &pst_axis_status->fPosition,      /*  7 */
                     &pst_axis_status->fAcceleration,  /*  8 */
                     &pst_axis_status->fDecceleration, /*  9 */
                     &pst_axis_status->bJogFwd,        /* 10 */
                     &pst_axis_status->bJogBwd,        /* 11 */
                     &pst_axis_status->bLimitFwd,      /* 12 */
                     &pst_axis_status->bLimitBwd,      /* 13 */
                     &pst_axis_status->fOverride,      /* 14 */
                     &pst_axis_status->bHomeSensor,    /* 15 */
                     &pst_axis_status->bEnabled,       /* 16 */
                     &pst_axis_status->bError,         /* 17 */
                     &pst_axis_status->nErrorId,       /* 18 */
                     &pst_axis_status->fActVelocity,   /* 19 */
                     &pst_axis_status->fActPosition,   /* 20 */
                     &pst_axis_status->fActDiff,       /* 21 */
                     &pst_axis_status->bHomed,         /* 22 */
                     &pst_axis_status->bBusy           /* 23 */);
    }
    if (nvals != 24) {
      goto pollAllWrongnvals;
    }

    /* V1 new style: mvnNRdyNex follows bBusy */
    if (pC_->features_ & (FEATURE_BITS_ECMC | FEATURE_BITS_SIM))
      drvlocal.supported.bV1BusyNewStyle = 1;

    pst_axis_status->mvnNRdyNex = pst_axis_status->bBusy && pst_axis_status->bEnabled;
    if (!drvlocal.supported.bV1BusyNewStyle) {
      /* "V1 old style":done when bEcecute is 0 */
      pst_axis_status->mvnNRdyNex &= pst_axis_status->bExecute;
    }
  } /* End of V1 */
  /* From here on, either V1 or V2 is supported */
  if (drvlocal.dirty.statusVer) {
    if (pC_->features_ & FEATURE_BITS_V2)
      drvlocal.supported.statusVer = 2;
    else if ((pC_->features_ & FEATURE_BITS_V1) && !drvlocal.supported.bV1BusyNewStyle)
      drvlocal.supported.statusVer = 0;
    else if ((pC_->features_ & FEATURE_BITS_V1) && drvlocal.supported.bV1BusyNewStyle)
      drvlocal.supported.statusVer = 1;
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spollAll(%d) nvals=%d V1=%d V2=%d sim=%d ecmc=%d bV1BusyNew=%d Ver=%d cmd/data=%d/%d fPos=%f fActPos=%f\n",
              modNamEMC, axisNo_, nvals,
              pC_->features_ & FEATURE_BITS_V1,
              pC_->features_ & FEATURE_BITS_V2,
              pC_->features_ & FEATURE_BITS_SIM,
              pC_->features_ & FEATURE_BITS_ECMC,
              drvlocal.supported.bV1BusyNewStyle,
              drvlocal.supported.statusVer,
              pst_axis_status->nCommand,
              pst_axis_status->nCmdData,
              pst_axis_status->fPosition,
              pst_axis_status->fActPosition);
#ifdef motorFlagsHomeOnLsString
    setIntegerParam(pC_->motorFlagsHomeOnLs_, 1);
#endif
#ifdef motorFlagsStopOnProblemString
    setIntegerParam(pC_->motorFlagsStopOnProblem_, 0);
#endif
    drvlocal.dirty.statusVer = 0;
  }
  if (axisNo_ != motor_axis_no) return asynError;

  /* Use previous fActPosition and current fActPosition to calculate direction.*/
  if (pst_axis_status->fActPosition > drvlocal.old_st_axis_status.fActPosition) {
    pst_axis_status->motorDiffPostion = 1;
    pst_axis_status->motorStatusDirection = 1;
  } else if (pst_axis_status->fActPosition < drvlocal.old_st_axis_status.fActPosition) {
    pst_axis_status->motorDiffPostion = 1;
    pst_axis_status->motorStatusDirection = 0;
  }
  return asynSuccess;


pollAllWrongnvals:
  /* rubbish on the line */
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
            "%spollAll(%d) nvals=%d out=%s in=%s \n",
            modNamEMC, axisNo_, nvals, pC_->outString_, pC_->inString_);
  return asynDisabled;
}


/** Polls the axis.
 * This function reads the motor position, the limit status, the home status, the moving status,
 * and the drive power-on status.
 * It calls setIntegerParam() and setDoubleParam() for each item that it polls,
 * and then calls callParamCallbacks() at the end.
 * \param[out] moving A flag that is set indicating that the axis is moving (true) or done (false). */
asynStatus EthercatMCAxis::poll(bool *moving)
{
  asynStatus comStatus = asynSuccess;
  st_axis_status_type st_axis_status;
  double timeBefore = EthercatMCgetNowTimeSecs();
#ifndef motorWaitPollsBeforeReadyString
  int waitNumPollsBeforeReady_ = drvlocal.waitNumPollsBeforeReady;
#endif

  if (drvlocal.supported.statusVer == -1) {
    callParamCallbacksUpdateError();
    return asynSuccess;
  }
  /* Driver not yet initialized, do nothing */
  if (!drvlocal.scaleFactor) return comStatus;

  memset(&st_axis_status, 0, sizeof(st_axis_status));
  comStatus = pollAll(moving, &st_axis_status);
  if (comStatus) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sout=%s in=%s return=%s (%d)\n",
              modNamEMC, pC_->outString_, pC_->inString_,
              EthercatMCstrStatus(comStatus), (int)comStatus);
    if (comStatus == asynDisabled) {
      return asynSuccess;
    }
    goto skip;
  }

  if (drvlocal.cfgDebug_str) {
    asynStatus comStatus;
    snprintf(pC_->outString_, sizeof(pC_->outString_), "%s", drvlocal.cfgDebug_str);
    comStatus = pC_->writeReadOnErrorDisconnect();
    if (!comStatus) {
      updateMsgTxtFromDriver(pC_->inString_);
    }
  }

  setIntegerParam(pC_->motorStatusHomed_, st_axis_status.bHomed);
  drvlocal.homed = st_axis_status.bHomed;
  setIntegerParam(pC_->motorStatusCommsError_, 0);
  setIntegerParam(pC_->motorStatusAtHome_, st_axis_status.bHomeSensor);
  setIntegerParam(pC_->motorStatusLowLimit_, !st_axis_status.bLimitBwd);
  setIntegerParam(pC_->motorStatusHighLimit_, !st_axis_status.bLimitFwd);
  setIntegerParam(pC_->motorStatusPowerOn_, st_axis_status.bEnabled);
  setDoubleParam(pC_->EthercatMCVelAct_, st_axis_status.fActVelocity);
  setDoubleParam(pC_->EthercatMCAcc_RB_, st_axis_status.fAcceleration);

#ifndef motorWaitPollsBeforeReadyString
  if (drvlocal.waitNumPollsBeforeReady) {
    *moving = true;
  }
  else
#endif
  {
    *moving = st_axis_status.mvnNRdyNex ? true : false;
    if (!st_axis_status.mvnNRdyNex &&
        !(pC_->features_ & FEATURE_BITS_ECMC)) {
      /* not moving: poll the parameters for this axis */
      int axisID = getMotionAxisID();
      switch (drvlocal.eeAxisPollNow) {
      case pollNowReadScaling:
        readScaling(axisID);
        drvlocal.eeAxisPollNow = pollNowReadMonitoring;
        break;
      case pollNowReadMonitoring:
        readMonitoring(axisID);
        drvlocal.eeAxisPollNow = pollNowReadBackSoftLimits;
        break;
      case pollNowReadBackSoftLimits:
        readBackSoftLimits();
        drvlocal.eeAxisPollNow = pollNowReadBackVelocities;
        break;
      case pollNowReadBackVelocities:
      default:
        readBackVelocities(axisID);
        drvlocal.eeAxisPollNow = pollNowReadScaling;
        break;
      }
    }
  }

  if (st_axis_status.mvnNRdyNex)
    drvlocal.nCommandActive = st_axis_status.nCommand;
  else {
    drvlocal.nCommandActive = 0;
    if (drvlocal.eeAxisWarning == eeAxisWarningSpeedLimit) {
      drvlocal.eeAxisWarning = eeAxisWarningNoWarning;
    }
  }

  if (drvlocal.nCommandActive != NCOMMANDHOME) {
    setDoubleParam(pC_->motorPosition_,
                   st_axis_status.fActPosition / drvlocal.scaleFactor);
    setDoubleParam(pC_->motorEncoderPosition_,
                   st_axis_status.fActPosition / drvlocal.scaleFactor);
    drvlocal.old_st_axis_status.fActPosition = st_axis_status.fActPosition;
    setDoubleParam(pC_->EthercatMCVel_RB_, st_axis_status.fVelocity);
  }

  if (drvlocal.externalEncoderStr) {
    comStatus = getValueFromController(drvlocal.externalEncoderStr,
                                       &st_axis_status.encoderRaw);
    if (!comStatus) setDoubleParam(pC_->EthercatMCEncAct_,
                                   st_axis_status.encoderRaw);
  } else if (pC_->features_ & FEATURE_BITS_V2) {
    setDoubleParam(pC_->EthercatMCEncAct_, st_axis_status.encoderRaw);
  }

  if (drvlocal.old_st_axis_status.bHomed != st_axis_status.bHomed) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spoll(%d) homed=%d\n",
              modNamEMC, axisNo_, st_axis_status.bHomed);
    drvlocal.old_st_axis_status.bHomed =  st_axis_status.bHomed;
  }
  if (drvlocal.old_st_axis_status.bLimitBwd != st_axis_status.bLimitBwd) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spoll(%d) LLS=%d\n",
              modNamEMC, axisNo_, !st_axis_status.bLimitBwd);
    drvlocal.old_st_axis_status.bLimitBwd =  st_axis_status.bLimitBwd;
  }
  if (drvlocal.old_st_axis_status.bLimitFwd != st_axis_status.bLimitFwd) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spoll(%d) HLS=%d\n",
              modNamEMC, axisNo_,!st_axis_status.bLimitFwd);
    drvlocal.old_st_axis_status.bLimitFwd = st_axis_status.bLimitFwd;
  }

#ifndef motorWaitPollsBeforeReadyString
  if (drvlocal.waitNumPollsBeforeReady) {
    /* Don't update moving, done, motorStatusProblem_ */
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spoll(%d) mvnNRdyNexAt=%d Ver=%d bBusy=%d bExecute=%d bEnabled=%d atTarget=%d waitNumPollsBeforeReady=%d\n",
               modNamEMC,
              axisNo_, st_axis_status.mvnNRdyNex,
              drvlocal.supported.statusVer,
              st_axis_status.bBusy, st_axis_status.bExecute,
              st_axis_status.bEnabled, st_axis_status.atTarget,
              drvlocal.waitNumPollsBeforeReady);
    drvlocal.waitNumPollsBeforeReady--;
    callParamCallbacks();
  }
  else
#endif
  {
    if (drvlocal.old_st_axis_status.mvnNRdyNex != st_axis_status.mvnNRdyNex ||
        drvlocal.old_st_axis_status.bBusy      != st_axis_status.bBusy ||
        drvlocal.old_st_axis_status.bEnabled   != st_axis_status.bEnabled ||
        drvlocal.old_st_axis_status.bExecute   != st_axis_status.bExecute ||
        drvlocal.old_st_axis_status.atTarget   != st_axis_status.atTarget) {
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
                "%spoll(%d) mvnNRdy=%d Ver=%d bBusy=%d bExe=%d bEnabled=%d atTarget=%d wf=%d ENC=%g fPos=%g fActPosition=%g time=%f\n",
                modNamEMC, axisNo_, st_axis_status.mvnNRdyNex,
                drvlocal.supported.statusVer,
                st_axis_status.bBusy, st_axis_status.bExecute,
                st_axis_status.bEnabled, st_axis_status.atTarget,
                waitNumPollsBeforeReady_,
                st_axis_status.encoderRaw, st_axis_status.fPosition,
                st_axis_status.fActPosition,
                EthercatMCgetNowTimeSecs() - timeBefore);
    }
  }
  setIntegerParam(pC_->motorStatusDirection_, st_axis_status.motorStatusDirection);
  setIntegerParam(pC_->motorStatusMoving_, st_axis_status.mvnNRdyNex);
  setIntegerParam(pC_->motorStatusDone_, !st_axis_status.mvnNRdyNex);

  drvlocal.MCU_nErrorId = st_axis_status.nErrorId;

  if (drvlocal.cfgDebug_str) {
    ; /* Do not do the following */
  } else if (drvlocal.old_bError != st_axis_status.bError ||
      drvlocal.old_MCU_nErrorId != drvlocal.MCU_nErrorId ||
      drvlocal.dirty.sErrorMessage) {
    char sErrorMessage[256];
    int nErrorId = st_axis_status.nErrorId;
    const char *errIdString = errStringFromErrId(nErrorId);
    sErrorMessage[0] = '\0';
    drvlocal.sErrorMessage[0] = '\0';
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%spoll(%d) bError=%d st_axis_status.nErrorId=0x%x\n",
               modNamEMC, axisNo_, st_axis_status.bError,
              nErrorId);
    drvlocal.old_bError = st_axis_status.bError;
    drvlocal.old_MCU_nErrorId = nErrorId;
    drvlocal.dirty.sErrorMessage = 0;
    if (nErrorId) {
      /* Get the ErrorMessage to have it in the log file */
      (void)getStringFromAxis("sErrorMessage", (char *)&sErrorMessage[0],
                              sizeof(sErrorMessage));
      asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
                "%ssErrorMessage(%d)=\"%s\"\n",
                modNamEMC, axisNo_, sErrorMessage);
    }
    /* First choice: "well known" ErrorIds */
    if (errIdString[0]) {
      snprintf(drvlocal.sErrorMessage, sizeof(drvlocal.sErrorMessage)-1, "E: %s %x",
               errIdString, nErrorId);
    } else if ((pC_->features_ & FEATURE_BITS_ECMC) && nErrorId) {
      /* emcmc has error messages */
      snprintf(drvlocal.sErrorMessage, sizeof(drvlocal.sErrorMessage)-1, "E: %s",
               sErrorMessage);
    } else if (nErrorId) {
      snprintf(drvlocal.sErrorMessage, sizeof(drvlocal.sErrorMessage)-1, "E: Cntrl Error %x", nErrorId);
    }
    /* The poller will update the MsgTxt field */
    // updateMsgTxtFromDriver(drvlocal.sErrorMessage);
  }
  callParamCallbacksUpdateError();

  memcpy(&drvlocal.old_st_axis_status, &st_axis_status,
         sizeof(drvlocal.old_st_axis_status));
  return asynSuccess;

  skip:
  return asynError;
}

/** Set the motor closed loop status
  * \param[in] closedLoop true = close loop, false = open looop. */
asynStatus EthercatMCAxis::setClosedLoop(bool closedLoop)
{
  int value = closedLoop ? 1 : 0;
  asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
            "%ssetClosedLoop(%d)=%d\n",  modNamEMC, axisNo_, value);
  if (drvlocal.axisFlags & AMPLIFIER_ON_FLAG_USING_CNEN) {
    return enableAmplifier(value);
  }
  return asynSuccess;
}

asynStatus EthercatMCAxis::setIntegerParam(int function, int value)
{
  asynStatus status;
  unsigned indexGroup5000 = 0x5000;
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
      drvlocal.MCU_nErrorId = 0;
    }
#ifdef motorPowerAutoOnOffString
  } else if (function == pC_->motorPowerAutoOnOff_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetIntegerParam(%d motorPowerAutoOnOff_)=%d\n", modNamEMC, axisNo_, value);
#endif
  } else if (function == pC_->EthercatMCHomProc_) {
    int motorNotHomedProblem = 0;
    setIntegerParam(pC_->EthercatMCHomProc_RB_, value);
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetIntegerParam(%d HomProc_)=%d motorNotHomedProblem=%d\n",
              modNamEMC, axisNo_, value, motorNotHomedProblem);
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
  } else if (function == pC_->EthercatMCCfgDHLM_En_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetIntegerParam(%d EthercatMCCfgDHLM_En)=%d\n",
              modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(indexGroup5000, 0xC, value);
    readBackSoftLimits();
    return status;
  } else if (function == pC_->EthercatMCCfgDLLM_En_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetIntegerParam(%d EthercatMCCfgDLLM_En)=%d\n",
              modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(indexGroup5000, 0xB, value);
    readBackSoftLimits();
    return status;
  }

  //Call base class method
  status = asynMotorAxis::setIntegerParam(function, value);
  return status;
}

/** Set a floating point parameter on the axis
 * \param[in] function, which parameter is updated
 * \param[in] value, the new value
 *
 * When the IOC starts, we will send the soft limits to the controller.
 * When a soft limit is changed, and update is send them to the controller.
 */
asynStatus EthercatMCAxis::setDoubleParam(int function, double value)
{
  asynStatus status;
  unsigned indexGroup5000 = 0x5000;

  if (function == pC_->motorMoveRel_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorMoveRel_)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorMoveAbs_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorMoveAbs_)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorMoveVel_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorMoveVel_)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorHome_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorHome__)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorStop_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorStop_)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorVelocity_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorVelocity_=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorVelBase_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorVelBase_)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorAccel_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorAccel_)=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorDeferMoves_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motmotorDeferMoves_=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorMoveToHome_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motmotorMoveToHome_=%g\n", modNamEMC, axisNo_, value);
  } else if (function == pC_->motorResolution_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorResolution_=%g\n",  modNamEMC, axisNo_, value);
    /* Limits handled above */

#ifdef motorPowerOnDelayString
  } else if (function == pC_->motorPowerOnDelay_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorPowerOnDelay_)=%g\n", modNamEMC, axisNo_, value);
#endif
#ifdef motorPowerOffDelayString
  } else if (function == pC_->motorPowerOffDelay_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorPowerOffDelay_=%g\n", modNamEMC, axisNo_, value);
#endif
#ifdef motorPowerOffFractionString
  } else if (function == pC_->motorPowerOffFraction_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motomotorPowerOffFraction_=%g\n", modNamEMC, axisNo_, value);
#endif
#ifdef motorPostMoveDelayString
  } else if (function == pC_->motorPostMoveDelay_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorPostMoveDelay_=%g\n", modNamEMC, axisNo_, value);
#endif
  } else if (function == pC_->motorStatus_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d motorStatus_)=%g\n", modNamEMC, axisNo_, value);
#ifdef EthercatMCHVELFRMString
  } else if (function == pC_->EthercatMCHVELfrm_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d HVELfrm_)=%g\n", modNamEMC, axisNo_, value);
#endif
#ifdef EthercatMCHomPosString
  } else if (function == pC_->EthercatMCHomPos_) {
    pC_->setDoubleParam(axisNo_, pC_->EthercatMCHomPos_RB_, value);
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d HomPos_)=%f\n", modNamEMC, axisNo_, value);
#endif
  } else if (function == pC_->EthercatMCCfgDHLM_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d EthercatMCCfgDHLM_)=%f\n", modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(indexGroup5000, 0xE, value);
    readBackSoftLimits();
    return status;
  } else if (function == pC_->EthercatMCCfgDLLM_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d EthercatMCCfgDLLM_)=%f\n", modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(indexGroup5000, 0xD, value);
    readBackSoftLimits();
    return status;
  } else if (function == pC_->EthercatMCCfgVELO_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d EthercatMCCfgVELO_)=%f\n", modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(0x4000, 0x9, value);
    return status;
  } else if (function == pC_->EthercatMCCfgVMAX_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d EthercatMCCfgVMAX_)=%f\n", modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(0x4000, 0x27, value);
    return status;
  } else if (function == pC_->EthercatMCCfgJVEL_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d EthercatMCCfgJVEL_)=%f\n", modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(0x4000, 0x8, value);
    return status;
  } else if (function == pC_->EthercatMCCfgACCS_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetDoubleParam(%d EthercatMCCfgACCS_)=%f\n", modNamEMC, axisNo_, value);
    status = setSAFValueOnAxis(0x4000, 0x101, value);
    return status;
  }
  // Call the base class method
  status = asynMotorAxis::setDoubleParam(function, value);
  return status;
}

asynStatus EthercatMCAxis::setStringParamDbgStrToMcu(const char *value)
{
    const char * const Main_this_str = "Main.this.";
    const char * const Sim_this_str = "Sim.this.";

    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetStringParamDbgStrToMcu(%d)=\"%s\"\n",
              modNamEMC, axisNo_, value);
    /* empty strings are not send to the controller */
    if (!value[0]) return asynSuccess;

    /* Check the string. E.g. Main.this. and Sim.this. are passed
       as Main.M1 or Sim.M1
       ADR commands are handled below */
    if (!strncmp(value, Main_this_str, strlen(Main_this_str))) {
      snprintf(pC_->outString_, sizeof(pC_->outString_), "%sMain.M%d.%s",
              drvlocal.adsport_str, axisNo_, value + strlen(Main_this_str));
      return pC_->writeReadACK(ASYN_TRACE_INFO);
    }
    /* caput IOC:m1-DbgStrToMCU Sim.this.log=M1.log */
    if (!strncmp(value, Sim_this_str, strlen(Sim_this_str))) {
      snprintf(pC_->outString_, sizeof(pC_->outString_), "Sim.M%d.%s",
              axisNo_, value + strlen(Sim_this_str));
      return pC_->writeReadACK(ASYN_TRACE_INFO);
    }
    /* If we come here, the command was not understood */
    return asynError;
}

asynStatus EthercatMCAxis::setStringParam(int function, const char *value)
{
  if (function == pC_->EthercatMCDbgStrToLog_) {
    asynPrint(pC_->pasynUserController_, ASYN_TRACE_INFO,
              "%ssetStringParamDbgStrToLog(%d)=\"%s\"\n",
              modNamEMC, axisNo_, value);
  } else if (function == pC_->EthercatMCDbgStrToMcu_) {
    return setStringParamDbgStrToMcu(value);
  }
  /* Call base class method */
  return asynMotorAxis::setStringParam(function, value);
}

#ifndef motorMessageTextString
void EthercatMCAxis::updateMsgTxtFromDriver(const char *value)
{
  if (value && value[0]) {
    setStringParam(pC_->EthercatMCMCUErrMsg_,value);
  } else {
    setStringParam(pC_->EthercatMCMCUErrMsg_, "");
  }
}
#endif
