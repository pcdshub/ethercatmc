/*
FILENAME... EthercatMCController.cpp
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <iocsh.h>
#include <epicsExit.h>
#include <epicsThread.h>

#include <asynOctetSyncIO.h>
#include <epicsExport.h>
#include "EthercatMCController.h"
#include "EthercatMCAxis.h"
#include "EthercatMCIndexerAxis.h"

#ifndef ASYN_TRACE_INFO
#define ASYN_TRACE_INFO      0x0040
#endif

const char *modNamEMC = "EthercatMC:: ";

const static char *const strEthercatMCCreateController = "EthercatMCCreateController";
const static char *const strEthercatMCConfigController = "EthercatMCConfigController";
const static char *const strEthercatMCConfigOrDie      = "EthercatMCConfigOrDie";
const static char *const strEthercatMCReadController   = "EthercatMCReadController";
const static char *const strEthercatMCCreateAxisDef    = "EthercatMCCreateAxis";
const static char *const strEthercatMCCreateIndexerAxisDef = "EthercatMCCreateIndexerAxis";
const static char *const strCtrlReset = ".ctrl.ErrRst";

const static char *const modulName = "EthercatMCAxis::";

extern "C"
double EthercatMCgetNowTimeSecs(void)
{
    epicsTimeStamp nowTime;
    epicsTimeGetCurrent(&nowTime);
    return nowTime.secPastEpoch + (nowTime.nsec * 0.000000001);
}

extern "C" const char *errStringFromErrId(int nErrorId)
{
  switch(nErrorId) {
  case 0x4221:
    return "Velo not allowed";
  case 0x4223:
    return "Axis positioning enable";
  case 0x4450:
  case 0x4451:
    return "Follow error";
  case 0x4260:
    return "Amplifier off";
  case 0x4263:
    return "Is still proc";
  case 0x42A0:
    return "Consequ Err";
  case 0x4460:
    return "Low soft limit";
  case 0x4461:
    return "High soft limit";
  case 0x4462:
    return "Min position";
  case 0x4463:
    return "Max position";
  case 0x4464:
    return "HW fault";
  case 0x4550:
    return "Follow err pos";
  case 0x4551:
    return "Follow err vel";
  case 0x4650:
    return "Drv HW not rdy";
  case 0x4655:
    return "Inv IO data";
  case 0x4B09:
    return "Axis not ready";
  case 0x4B0A:
    return "Homing failed";
  default:
    return "";
  }
}



/** Creates a new EthercatMCController object.
  * \param[in] portName          The name of the asyn port that will be created for this driver
  * \param[in] MotorPortName     The name of the drvAsynSerialPort that was created previously to connect to the EthercatMC controller
  * \param[in] numAxes           The number of axes that this controller supports
  * \param[in] movingPollPeriod  The time between polls when any axis is moving
  * \param[in] idlePollPeriod    The time between polls when no axis is moving
  */
EthercatMCController::EthercatMCController(const char *portName,
                                           const char *MotorPortName, int numAxes,
                                           double movingPollPeriod,
                                           double idlePollPeriod,
                                           const char *optionStr)
  :  asynMotorController(portName, numAxes, NUM_VIRTUAL_MOTOR_PARAMS,
                         0, // No additional interfaces beyond those in base class
                         0, // No additional callback interfaces beyond those in base class
                         ASYN_CANBLOCK | ASYN_MULTIDEVICE,
                         1, // autoconnect
                         0, 0)  // Default priority and stack size
{
  asynStatus status;

  /* Controller */
  memset(&ctrlLocal, 0, sizeof(ctrlLocal));
  features_ = 0;
#ifndef motorMessageTextString
  createParam("MOTOR_MESSAGE_TEXT",          asynParamOctet,       &EthercatMCMCUErrMsg_);
#else
  createParam(EthercatMCMCUErrMsgString,     asynParamOctet,       &EthercatMCMCUErrMsg_);
#endif
  createParam(EthercatMCDbgStrToMcuString,   asynParamOctet,       &EthercatMCDbgStrToMcu_);

  /* Per axis */
  createParam(EthercatMCErrString,           asynParamInt32,       &EthercatMCErr_);
  createParam(EthercatMCErrIdString,         asynParamInt32,       &EthercatMCErrId_);

  createParam(EthercatMCEnc_ActString,       asynParamFloat64,     &EthercatMCEncAct_);
  createParam(EthercatMCHomProcString,       asynParamInt32,       &EthercatMCHomProc_);
  createParam(EthercatMCHomPosString,        asynParamFloat64,     &EthercatMCHomPos_);
  createParam(EthercatMCStatusCodeString,    asynParamInt32,       &EthercatMCStatusCode_);
  createParam(EthercatMCStatusBitsString,    asynParamInt32,       &EthercatMCStatusBits_);
  createParam(EthercatMCHomProc_RBString,    asynParamInt32,       &EthercatMCHomProc_RB_);
  createParam(EthercatMCHomPos_RBString,     asynParamFloat64,     &EthercatMCHomPos_RB_);
  createParam(EthercatMCVelToHomString,      asynParamFloat64,     &EthercatMCVelToHom_);
  createParam(EthercatMCVelFrmHomString,     asynParamFloat64,     &EthercatMCVelFrmHom_);
  createParam(EthercatMCAccHomString,        asynParamFloat64,     &EthercatMCAccHom_);
  createParam(EthercatMCErrRstString,        asynParamInt32,       &EthercatMCErrRst_);
  createParam(EthercatMCVelActString,        asynParamFloat64,     &EthercatMCVelAct_);
  createParam(EthercatMCVel_RBString,        asynParamFloat64,     &EthercatMCVel_RB_);
  createParam(EthercatMCAcc_RBString,        asynParamFloat64,     &EthercatMCAcc_RB_);
  createParam(EthercatMCaux0_String,         asynParamOctet,       &EthercatMCaux0_);
  createParam(EthercatMCaux1_String,         asynParamOctet,       &EthercatMCaux1_);
  createParam(EthercatMCaux2_String,         asynParamOctet,       &EthercatMCaux2_);
  createParam(EthercatMCaux3_String,         asynParamOctet,       &EthercatMCaux3_);
  createParam(EthercatMCaux4_String,         asynParamOctet,       &EthercatMCaux4_);
  createParam(EthercatMCaux5_String,         asynParamOctet,       &EthercatMCaux5_);
  createParam(EthercatMCaux6_String,         asynParamOctet,       &EthercatMCaux6_);
  createParam(EthercatMCaux7_String,         asynParamOctet,       &EthercatMCaux7_);
  createParam(EthercatMCreason8_String,      asynParamOctet,       &EthercatMCreason8_);
  createParam(EthercatMCreason9_String,      asynParamOctet,       &EthercatMCreason9_);
  createParam(EthercatMCreason10_String,     asynParamOctet,       &EthercatMCreason10_);
  createParam(EthercatMCreason11_String,     asynParamOctet,       &EthercatMCreason11_);
  createParam(EthercatMCCfgVELO_String,      asynParamFloat64,     &EthercatMCCfgVELO_);
  createParam(EthercatMCCfgVMAX_String,      asynParamFloat64,     &EthercatMCCfgVMAX_);
  createParam(EthercatMCCfgJVEL_String,      asynParamFloat64,     &EthercatMCCfgJVEL_);
  createParam(EthercatMCCfgACCS_String,      asynParamFloat64,     &EthercatMCCfgACCS_);
  createParam(EthercatMCCfgDHLMString,       asynParamFloat64,     &EthercatMCCfgDHLM_);
  createParam(EthercatMCCfgDLLMString,       asynParamFloat64,     &EthercatMCCfgDLLM_);
  createParam(EthercatMCCfgDHLM_EnString,    asynParamInt32,       &EthercatMCCfgDHLM_En_);
  createParam(EthercatMCCfgDLLM_EnString,    asynParamInt32,       &EthercatMCCfgDLLM_En_);

  createParam(EthercatMCCfgSREV_RBString,    asynParamFloat64,     &EthercatMCCfgSREV_RB_);
  createParam(EthercatMCCfgUREV_RBString,    asynParamFloat64,     &EthercatMCCfgUREV_RB_);
  createParam(EthercatMCCfgSPDB_RBString,    asynParamFloat64,     &EthercatMCCfgSPDB_RB_);
  createParam(EthercatMCCfgRDBD_RBString,    asynParamFloat64,     &EthercatMCCfgRDBD_RB_);
  createParam(EthercatMCCfgRDBD_Tim_RBString,asynParamFloat64,     &EthercatMCCfgRDBD_Tim_RB_);
  createParam(EthercatMCCfgRDBD_En_RBString, asynParamInt32,       &EthercatMCCfgRDBD_En_RB_);
  createParam(EthercatMCCfgPOSLAG_RBString,  asynParamFloat64,     &EthercatMCCfgPOSLAG_RB_);
  createParam(EthercatMCCfgPOSLAG_Tim_RBString,asynParamFloat64,   &EthercatMCCfgPOSLAG_Tim_RB_);
  createParam(EthercatMCCfgPOSLAG_En_RBString, asynParamInt32,     &EthercatMCCfgPOSLAG_En_RB_);

  createParam(EthercatMCCfgDESC_RBString,    asynParamOctet,       &EthercatMCCfgDESC_RB_);
  createParam(EthercatMCCfgEGU_RBString,     asynParamOctet,       &EthercatMCCfgEGU_RB_);

#ifdef CREATE_MOTOR_REC_RESOLUTION
  /* Latest asynMotorController does this, but not the version in 6.81 (or 6.9x) */
  createParam(motorRecResolutionString,        asynParamFloat64,      &motorRecResolution_);
  createParam(motorRecDirectionString,           asynParamInt32,      &motorRecDirection_);
  createParam(motorRecOffsetString,            asynParamFloat64,      &motorRecOffset_);
#endif

  /* Connect to EthercatMC controller */
  status = pasynOctetSyncIO->connect(MotorPortName, 0, &pasynUserController_, NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
              "%s cannot connect to motor controller\n", modulName);
  }
  printf("%s:%d %s optionStr=\"%s\"\n",
         __FILE__, __LINE__,
         modulName, optionStr ? optionStr : "NULL");
  if (optionStr && optionStr[0]) {
    const char * const adsPort_str        = "adsPort=";
    const char * const amsNetIdRemote_str = "amsNetIdRemote=";
    const char * const amsNetIdLocal_str  = "amsNetIdLocal=";
    char *pOptions = strdup(optionStr);
    char *pThisOption = pOptions;
    char *pNextOption = pOptions;
    int hasRemoteAmsNetId = 0;
    int hasLocalAmsNetId = 0;
    while (pNextOption && pNextOption[0]) {
      pNextOption = strchr(pNextOption, ';');
      if (pNextOption) {
        *pNextOption = '\0'; /* Terminate */
        pNextOption++;       /* Jump to (possible) next */
      }
      if (!strncmp(pThisOption, adsPort_str, strlen(adsPort_str))) {
        pThisOption += strlen(adsPort_str);
        int adsPort = atoi(pThisOption);
        if (adsPort > 0) {
          ctrlLocal.adsport = (unsigned)adsPort;
        }
      } else if (!strncmp(pThisOption, amsNetIdRemote_str,
                          strlen(amsNetIdRemote_str))) {
        AmsNetidAndPortType ams_netid_port;
        int nvals;
        memset(&ams_netid_port, 0, sizeof(ams_netid_port));
        pThisOption += strlen(amsNetIdRemote_str);
        nvals = sscanf(pThisOption, "%hhu.%hhu.%hhu.%hhu.%hhu.%hhu",
                       &ams_netid_port.netID[0],
                       &ams_netid_port.netID[1],
                       &ams_netid_port.netID[2],
                       &ams_netid_port.netID[3],
                       &ams_netid_port.netID[4],
                       &ams_netid_port.netID[5]);
        printf("%s:%d %s amsNetIdRemote=%u.%u.%u.%u.%u.%u:%u\n",
               __FILE__, __LINE__,
               modulName,
               ams_netid_port.netID[0], ams_netid_port.netID[1],
               ams_netid_port.netID[2], ams_netid_port.netID[3],
               ams_netid_port.netID[4], ams_netid_port.netID[5],
               ctrlLocal.adsport);
        if (nvals == 6) {
          hasRemoteAmsNetId = 1;
          ams_netid_port.port_low  = (uint8_t)ctrlLocal.adsport;
          ams_netid_port.port_high = (uint8_t)(ctrlLocal.adsport >> 8);
          memcpy(&ctrlLocal.remote, &ams_netid_port, sizeof(ctrlLocal.remote));
        }
      } else if (!strncmp(pThisOption, amsNetIdLocal_str,
                          strlen(amsNetIdLocal_str))) {
        AmsNetidAndPortType ams_netid_port;
        int nvals;
        memset(&ams_netid_port, 0, sizeof(ams_netid_port));
        pThisOption += strlen(amsNetIdLocal_str);
        nvals = sscanf(pThisOption, "%hhu.%hhu.%hhu.%hhu.%hhu.%hhu",
                       &ams_netid_port.netID[0],
                       &ams_netid_port.netID[1],
                       &ams_netid_port.netID[2],
                       &ams_netid_port.netID[3],
                       &ams_netid_port.netID[4],
                       &ams_netid_port.netID[5]);
        printf("%s:%d %s amsNetIdLocal=%u.%u.%u.%u.%u.%u:%u\n",
               __FILE__, __LINE__,
               modulName,
               ams_netid_port.netID[0], ams_netid_port.netID[1],
               ams_netid_port.netID[2], ams_netid_port.netID[3],
               ams_netid_port.netID[4], ams_netid_port.netID[5],
               ctrlLocal.adsport);
        if (nvals == 6) {
          hasLocalAmsNetId = 1;
          ams_netid_port.port_low  = (uint8_t)ctrlLocal.adsport;
          ams_netid_port.port_high = (uint8_t)(ctrlLocal.adsport >> 8);
          memcpy(&ctrlLocal.local, &ams_netid_port, sizeof(ctrlLocal.local));
        }
      }
      pThisOption = pNextOption;
    }
    free(pOptions);
    if (hasRemoteAmsNetId && hasLocalAmsNetId) {
      ctrlLocal.useADSbinary = 1;
    }
  }
  asynPrint(this->pasynUserSelf, ASYN_TRACE_INFO,
            "%s optionStr=\"%s\"\n",
            modulName, optionStr ? optionStr : "NULL");

  startPoller(movingPollPeriod, idlePollPeriod, 2);
}


/** Creates a new EthercatMCController object.
  * Configuration command, called directly or from iocsh
  * \param[in] portName          The name of the asyn port that will be created for this driver
  * \param[in] MotorPortName  The name of the drvAsynIPPPort that was created previously to connect to the EthercatMC controller
  * \param[in] numAxes           The number of axes that this controller supports (0 is not used)
  * \param[in] movingPollPeriod  The time in ms between polls when any axis is moving
  * \param[in] idlePollPeriod    The time in ms between polls when no axis is moving
  */
extern "C" int EthercatMCCreateController(const char *portName,
                                          const char *MotorPortName,
                                          int numAxes,
                                          int movingPollPeriod,
                                          int idlePollPeriod,
                                          const char *optionStr)
{
  new EthercatMCController(portName, MotorPortName, 1+numAxes,
                           movingPollPeriod/1000., idlePollPeriod/1000.,
                           optionStr);
  return(asynSuccess);
}

/** Writes a string to the controller and reads a response.
  * Disconnects in case of error
  */
extern "C" int EthercatMCConfigController(int needOkOrDie, const char *portName,
                                          const char *configStr)
{
  EthercatMCController *pC;

  if (!portName || !configStr)  {
    printf("%sNULL parameter\n", strEthercatMCConfigController);
    return asynError;
  }
  pC = (EthercatMCController*) findAsynPortDriver(portName);
  if (!pC) {
    printf("%s:%s: Error port %s not found\n",
           __FILE__, __FUNCTION__, portName);
    return asynError;
  }
  return pC->configController(needOkOrDie, configStr);
}

asynStatus EthercatMCController::configController(int needOkOrDie, const char *value)
{
  char inString[MAX_CONTROLLER_STRING_SIZE];
  size_t configStrLen = strlen(value);
  asynStatus status = asynError;

  if (!strcmp(value, strCtrlReset)) {
    ctrlLocal.hasConfigError = 0;
    setMCUErrMsg("OK");
    return asynSuccess;
  }
  if (ctrlLocal.hasConfigError) {
    printf("port %s has errors. To reset use\n %s %s %s \n",
           portName, strEthercatMCConfigController, portName, strCtrlReset);
    return asynError;
  }

  status = writeReadOnErrorDisconnect_C(pasynUserController_,
                                        value, configStrLen,
                                        inString, sizeof(inString));
  inString[sizeof(inString) -1] = '\0';
  if (status) {
    ctrlLocal.isConnected = 0;
  } else if (needOkOrDie) {
    status = checkACK(value, configStrLen, inString);
    if (status) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%s out=%s in=\"%s\" return=%s (%d)\n",
                modulName, value, inString,
                pasynManager->strStatus(status), (int)status);
      if (needOkOrDie < 0) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s Aborting IOC\n",
                  modulName);
        epicsExit(EXIT_FAILURE);
      }
      ctrlLocal.hasConfigError = 1;
      (void)setMCUErrMsg(inString);
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_INFO,
                "%s out=%s in=\"%s\"\n",
                modulName, value, inString);
    }
  } /* needOkOrDie */

  printf("%s\n", inString);
  return status;
}

extern "C" asynStatus disconnect_C(asynUser *pasynUser)
{
  asynStatus status = asynError;
  asynInterface *pasynInterface = NULL;
  asynCommon     *pasynCommon = NULL;
  pasynInterface = pasynManager->findInterface(pasynUser,
                                               asynCommonType,
                                               0 /* FALSE */);
  if (pasynInterface) {
    pasynCommon = (asynCommon *)pasynInterface->pinterface;
    status = pasynCommon->disconnect(pasynInterface->drvPvt,
                                       pasynUser);
    if (status != asynSuccess) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%s status=%s (%d)\n",
                modulName, pasynManager->strStatus(status), (int)status);
    }
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s pasynInterface=%p pasynCommon=%p\n",
              modulName, pasynInterface, pasynCommon);
  }
  return status;
}

extern "C"
asynStatus writeReadOnErrorDisconnect_C(asynUser *pasynUser,
                                        const char *outdata, size_t outlen,
                                        char *indata, size_t inlen)
{
  size_t nwrite;
  asynStatus status = asynError;
  int eomReason = 0;
  size_t nread;
  status = pasynOctetSyncIO->writeRead(pasynUser, outdata, outlen,
                                       indata, inlen,
                                       DEFAULT_CONTROLLER_TIMEOUT,
                                       &nwrite, &nread, &eomReason);
  if ((status == asynTimeout) ||
      (!nread && (eomReason & ASYN_EOM_END))) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s out=%s nread=%lu eomReason=%x (%s%s%s) status=%s (%d)\n",
              modulName, outdata,(unsigned long)nread,
              eomReason,
              eomReason & ASYN_EOM_CNT ? "CNT" : "",
              eomReason & ASYN_EOM_EOS ? "EOS" : "",
              eomReason & ASYN_EOM_END ? "END" : "",
              pasynManager->strStatus(status), status);
    disconnect_C(pasynUser);
    return asynError; /* TimeOut -> Error */
  }
  return status;
}

/** Writes a string to the controller and reads a response.
  * Disconnects in case of error
  */
asynStatus EthercatMCController::writeReadOnErrorDisconnect(void)
{
  asynStatus status = asynError;
  size_t outlen = strlen(outString_);
  double timeBefore = EthercatMCgetNowTimeSecs();
  inString_[0] = '\0';
  status = writeReadOnErrorDisconnect_C(pasynUserController_, outString_, outlen,
                                        inString_, sizeof(inString_));
  if (!status) {
    if (strstr(inString_, "State timout") ||
        strstr(inString_, "To long time in one state")) {
      double timeDelta = EthercatMCgetNowTimeSecs() - timeBefore;

      asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%sout=%s in=%s timeDelta=%f\n",
                modNamEMC, outString_, inString_, timeDelta);
      /* Try again */
      timeBefore = EthercatMCgetNowTimeSecs();
      status = writeReadOnErrorDisconnect_C(pasynUserController_,
                                            outString_, outlen,
                                            inString_, sizeof(inString_));
      timeDelta = EthercatMCgetNowTimeSecs() - timeBefore;
      asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                "%sout=%s in=%s timeDelta=%f status=%s (%d)\n",
                modNamEMC, outString_, inString_, timeDelta,
                pasynManager->strStatus(status), (int)status);
    }
  }
  handleStatusChange(status);
  if (status)
  {
    return asynError;
  }
  return status;
}

extern "C"
asynStatus checkACK(const char *outdata, size_t outlen,
                    const char *indata)
{
  size_t i;
  unsigned int numOK = 1;
  int res = 1;
  for( i = 0; i < outlen; i++) {
    if (outdata[i] == ';') numOK++;
  }
  switch(numOK) {
    case 1: res = strcmp(indata, "OK");  break;
    case 2: res = strcmp(indata, "OK;OK");  break;
    case 3: res = strcmp(indata, "OK:OK;OK");  break;
    case 4: res = strcmp(indata, "OK;OK;OK;OK");  break;
    default:
    ;
  }
  return res ? asynError : asynSuccess;
}

asynStatus EthercatMCController::writeReadControllerPrint(int traceMask)
{
  asynStatus status = writeReadOnErrorDisconnect();
  if (status && ctrlLocal.isConnected) traceMask |= ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER;
  asynPrint(pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, outString_, inString_,
            pasynManager->strStatus(status), (int)status);
  return status;
}

asynStatus EthercatMCController::writeReadACK(int traceMask)
{
  asynStatus status = writeReadOnErrorDisconnect();
  switch (status) {
    case asynError:
      return status;
    case asynSuccess:
    {
      const char *semicolon = &outString_[0];
      unsigned int numOK = 1;
      int res = 1;
      while (semicolon && semicolon[0]) {
        semicolon = strchr(semicolon, ';');
        if (semicolon) {
          numOK++;
          semicolon++;
        }
      }
      switch(numOK) {
        case 1: res = strcmp(inString_, "OK");  break;
        case 2: res = strcmp(inString_, "OK;OK");  break;
        case 3: res = strcmp(inString_, "OK:OK;OK");  break;
        case 4: res = strcmp(inString_, "OK;OK;OK;OK");  break;
        case 5: res = strcmp(inString_, "OK;OK;OK;OK;OK");  break;
        case 6: res = strcmp(inString_, "OK;OK;OK;OK;OK;OK");  break;
        case 7: res = strcmp(inString_, "OK;OK;OK;OK;OK;OK;OK");  break;
        case 8: res = strcmp(inString_, "OK;OK;OK;OK;OK;OK;OK;OK");  break;
        default:
          ;
      }
      if (res) {
        status = asynError;
        asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
                  "%sout=%s in=%s return=%s (%d)\n",
                  modNamEMC, outString_, inString_,
                  pasynManager->strStatus(status), (int)status);
        return status;
      }
    }
    default:
      break;
  }
  asynPrint(pasynUserController_, traceMask,
            "%sout=%s in=%s status=%s (%d)\n",
            modNamEMC, outString_, inString_,
            pasynManager->strStatus(status), (int)status);
  return status;
}

asynStatus EthercatMCController::setMCUErrMsg(const char *value)
{
  asynStatus status = setStringParam(EthercatMCMCUErrMsg_, value);
  if (!status) status = callParamCallbacks();
  return status;
}

void EthercatMCController::handleStatusChange(asynStatus status)
{

  if ((status && ctrlLocal.isConnected) ||
      (status == asynTimeout)) {
    /* Connected -> Disconnected */
    int i;
    ctrlLocal.isConnected = 0;
    ctrlLocal.initialPollDone = 0;
    /* Gvl comes via EthercatMCCreateAxis,
       keep that bit  */
    features_ &= ~FEATURE_BITS_GVL;
    setMCUErrMsg("MCU Disconnected");
    for (i=0; i<numAxes_; i++) {
      asynMotorAxis *pAxis=getAxis(i);
      if (!pAxis) continue;
      pAxis->setIntegerParam(motorStatusCommsError_, 1);
      pAxis->callParamCallbacks();
    }
  } else if (!status && !ctrlLocal.isConnected) {
    /* Disconnected -> Connected */
    ctrlLocal.isConnected = 1;
    setMCUErrMsg("MCU Cconnected");
  }
}

asynStatus EthercatMCController::poll(void)
{
  asynStatus status = asynSuccess;

  asynPrint(pasynUserController_, ASYN_TRACE_FLOW,
            "%spoll ctrlLocal.initialPollDone=%d\n",
            modNamEMC, ctrlLocal.initialPollDone);
  if (ctrlLocal.useADSbinary) {
    if (!ctrlLocal.initialPollDone) {
      status = initialPollIndexer();
      if (!status) ctrlLocal.initialPollDone = 1;
    }
  } else {
    if (!features_) {
      features_ = getFeatures();
    }
  }
  return status;
}

int EthercatMCController::getFeatures(void)
{
  /* The features we know about */
  const char * const sim_str = "sim";
  const char * const stECMC_str = "ecmc";
  const char * const stV1_str = "stv1";
  const char * const stV2_str = "stv2";
  const char * const ads_str = "ads";
  static const unsigned adsports[] = {851, 852, 853};
  unsigned adsport_idx;
  int ret = 0;
  for (adsport_idx = 0;
       adsport_idx < sizeof(adsports)/sizeof(adsports[0]);
       adsport_idx++) {
    unsigned adsport = adsports[adsport_idx];

    asynStatus status = asynSuccess;
    snprintf(outString_, sizeof(outString_),
             "ADSPORT=%u/.THIS.sFeatures?",
             adsport);
    inString_[0] = 0;
    status = writeReadOnErrorDisconnect();
    asynPrint(pasynUserController_, ASYN_TRACE_INFO,
              "%sout=%s in=%s status=%s (%d)\n",
              modNamEMC, outString_, inString_,
              pasynManager->strStatus(status), (int)status);
    if (!status) {
      /* loop through the features */
      char *pFeatures = strdup(inString_);
      char *pThisFeature = pFeatures;
      char *pNextFeature = pFeatures;

      while (pNextFeature && pNextFeature[0]) {
        pNextFeature = strchr(pNextFeature, ';');
        if (pNextFeature) {
          *pNextFeature = '\0'; /* Terminate */
          pNextFeature++;       /* Jump to (possible) next */
        }
        if (!strcmp(pThisFeature, sim_str)) {
          ret |= FEATURE_BITS_SIM;
        } else if (!strcmp(pThisFeature, stECMC_str)) {
          ret |= FEATURE_BITS_ECMC;
        } else if (!strcmp(pThisFeature, stV1_str)) {
          ret |= FEATURE_BITS_V1;
        } else if (!strcmp(pThisFeature, stV2_str)) {
          ret |= FEATURE_BITS_V2;
        } else if (!strcmp(pThisFeature, ads_str)) {
          ret |= FEATURE_BITS_ADS;
        }
        pThisFeature = pNextFeature;
      }
      free(pFeatures);
      if (ret) {
        asynPrint(pasynUserController_, ASYN_TRACE_INFO,
                  "%sout=%s in=%s ret=%d\n",
                  modNamEMC, outString_, inString_,
                  ret);
        /* Found something useful on this adsport */
        return ret;
      }
    }
  }
  return 0;
}


/** Reports on status of the driver
  * \param[in] fp The file pointer on which report information will be written
  * \param[in] level The level of report detail desired
  *
  * If details > 0 then information is printed about each axis.
  * After printing controller-specific information it calls asynMotorController::report()
  */
void EthercatMCController::report(FILE *fp, int level)
{
  fprintf(fp, "Twincat motor driver %s, numAxes=%d, moving poll period=%f, idle poll period=%f\n",
    this->portName, numAxes_, movingPollPeriod_, idlePollPeriod_);

  // Call the base class method
  asynMotorController::report(fp, level);
}

/** Returns a pointer to an EthercatMCAxis object.
  * Returns NULL if the axis number encoded in pasynUser is invalid.
  * \param[in] pasynUser asynUser structure that encodes the axis index number. */
EthercatMCAxis* EthercatMCController::getAxis(asynUser *pasynUser)
{
  return static_cast<EthercatMCAxis*>(asynMotorController::getAxis(pasynUser));
}

/** Returns a pointer to an EthercatMCAxis object.
  * Returns NULL if the axis number encoded in pasynUser is invalid.
  * \param[in] axisNo Axis index number. */
EthercatMCAxis* EthercatMCController::getAxis(int axisNo)
{
  return static_cast<EthercatMCAxis*>(asynMotorController::getAxis(axisNo));
}


/** Code for iocsh registration */
static const iocshArg EthercatMCCreateControllerArg0 = {"Port name", iocshArgString};
static const iocshArg EthercatMCCreateControllerArg1 = {"EPICS ASYN TCP motor port name", iocshArgString};
static const iocshArg EthercatMCCreateControllerArg2 = {"Number of axes", iocshArgInt};
static const iocshArg EthercatMCCreateControllerArg3 = {"Moving poll period (ms)", iocshArgInt};
static const iocshArg EthercatMCCreateControllerArg4 = {"Idle poll period (ms)", iocshArgInt};
static const iocshArg EthercatMCCreateControllerArg5 = {"options", iocshArgString};
static const iocshArg *const EthercatMCCreateControllerArgs[] = {&EthercatMCCreateControllerArg0,
                                                            &EthercatMCCreateControllerArg1,
                                                            &EthercatMCCreateControllerArg2,
                                                            &EthercatMCCreateControllerArg3,
                                                            &EthercatMCCreateControllerArg4,
                                                            &EthercatMCCreateControllerArg5};
static const iocshFuncDef EthercatMCCreateControllerDef = {strEthercatMCCreateController, 6,
                                                           EthercatMCCreateControllerArgs};
static void EthercatMCCreateContollerCallFunc(const iocshArgBuf *args)
{
  EthercatMCCreateController(args[0].sval, args[1].sval, args[2].ival,
                             args[3].ival, args[4].ival, args[5].sval);
}

/* EthercatMCConfigController */
static const iocshArg EthercatMCConfigControllerArg0 = {"Controller port name", iocshArgString};
static const iocshArg EthercatMCConfigControllerArg1 = {"ConfigString",         iocshArgString};
static const iocshArg * const EthercatMCConfigControllerArgs[] = {&EthercatMCConfigControllerArg0,
                                                                  &EthercatMCConfigControllerArg1};
static const iocshFuncDef EthercatMCConfigControllerDef = {strEthercatMCConfigController, 2,
                                                           EthercatMCConfigControllerArgs};
static const iocshFuncDef EthercatMCConfigOrDieDef = {strEthercatMCConfigOrDie, 2,
                                                      EthercatMCConfigControllerArgs};
static const iocshFuncDef EthercatMCReadControllerDef = {strEthercatMCReadController, 2,
                                                         EthercatMCConfigControllerArgs};

static void EthercatMCConfigContollerCallFunc(const iocshArgBuf *args)
{
  int needOkOrDie = 1;
  EthercatMCConfigController(needOkOrDie, args[0].sval, args[1].sval);
}

static void EthercatMCConfigOrDieCallFunc(const iocshArgBuf *args)
{
  int needOkOrDie = -1;
  EthercatMCConfigController(needOkOrDie, args[0].sval, args[1].sval);
}

static void EthercatMCReadContollerCallFunc(const iocshArgBuf *args)
{
  int needOkOrDie = 0;
  EthercatMCConfigController(needOkOrDie, args[0].sval, args[1].sval);
}

/* EthercatMCCreateAxis */
static const iocshArg EthercatMCCreateAxisArg0 = {"Controller port name", iocshArgString};
static const iocshArg EthercatMCCreateAxisArg1 = {"Axis number", iocshArgInt};
static const iocshArg EthercatMCCreateAxisArg2 = {"axisFlags", iocshArgInt};
static const iocshArg EthercatMCCreateAxisArg3 = {"axisOptionsStr", iocshArgString};
static const iocshArg * const EthercatMCCreateAxisArgs[] = {&EthercatMCCreateAxisArg0,
                                                            &EthercatMCCreateAxisArg1,
                                                            &EthercatMCCreateAxisArg2,
                                                            &EthercatMCCreateAxisArg3};
static const
iocshArg * const EthercatMCCreateIndexerAxisArgs[] = {&EthercatMCCreateAxisArg0,
                                                      &EthercatMCCreateAxisArg1,
                                                      &EthercatMCCreateAxisArg2,
                                                      &EthercatMCCreateAxisArg3};

static const iocshFuncDef EthercatMCCreateAxisDef = {strEthercatMCCreateAxisDef, 4,
                                                     EthercatMCCreateAxisArgs};

static const iocshFuncDef EthercatMCCreateIndexerAxisDef = {strEthercatMCCreateIndexerAxisDef, 4,
                                                     EthercatMCCreateIndexerAxisArgs};

static void EthercatMCCreateAxisCallFunc(const iocshArgBuf *args)
{
  EthercatMCCreateAxis(args[0].sval, args[1].ival, args[2].ival, args[3].sval);
}

static void EthercatMCCreateIndexerAxisCallFunc(const iocshArgBuf *args)
{
  EthercatMCCreateIndexerAxis(args[0].sval, args[1].ival, args[2].ival, args[3].sval);
}

static void EthercatMCControllerRegister(void)
{
  iocshRegister(&EthercatMCCreateControllerDef, EthercatMCCreateContollerCallFunc);
  iocshRegister(&EthercatMCConfigOrDieDef,      EthercatMCConfigOrDieCallFunc);
  iocshRegister(&EthercatMCConfigControllerDef, EthercatMCConfigContollerCallFunc);
  iocshRegister(&EthercatMCReadControllerDef,   EthercatMCReadContollerCallFunc);
  iocshRegister(&EthercatMCCreateAxisDef,       EthercatMCCreateAxisCallFunc);
  iocshRegister(&EthercatMCCreateIndexerAxisDef,EthercatMCCreateIndexerAxisCallFunc);
}

extern "C" {
  epicsExportRegistrar(EthercatMCControllerRegister);
}
