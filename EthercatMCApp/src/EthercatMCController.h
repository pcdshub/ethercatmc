/*
FILENAME...   EthercatMCController.h
*/

#ifndef ETHERCATMCCONTROLLER_H
#define ETHERCATMCCONTROLLER_H

#include "asynMotorController.h"
#include "asynMotorAxis.h"
#include "EthercatMCAxis.h"
#include "EthercatMCADSdefs.h"

#ifndef motorRecResolutionString
#define CREATE_MOTOR_REC_RESOLUTION
#define motorRecDirectionString         "MOTOR_REC_DIRECTION"
#define motorRecOffsetString            "MOTOR_REC_OFFSET"
#define motorRecResolutionString        "MOTOR_REC_RESOLUTION"
#endif

#define EthercatMCErrString                  "MCUErr"
#define EthercatMCErrIdString                "ErrId"
#define EthercatMCStatusCodeString           "StatusCode"
#define EthercatMCStatusBitsString           "StatusBits"
#define EthercatMCaux0_String                "AuxBit0"
#define EthercatMCaux1_String                "AuxBit1"
#define EthercatMCaux2_String                "AuxBit2"
#define EthercatMCaux3_String                "AuxBit3"
#define EthercatMCaux4_String                "AuxBit4"
#define EthercatMCaux5_String                "AuxBit5"
#define EthercatMCaux6_String                "AuxBit6"
#define EthercatMCaux7_String                "AuxBit7"
#define EthercatMCreason8_String             "ReasonBit8"
#define EthercatMCreason9_String             "ReasonBit9"
#define EthercatMCreason10_String            "ReasonBit10"
#define EthercatMCreason11_String            "ReasonBit11"
#define EthercatMCHomProc_RBString           "HomProc-RB"
#define EthercatMCHomPos_RBString            "HomPos-RB"
#define EthercatMCHomProcString              "HomProc"
#define EthercatMCHomPosString               "HomPos"
#define EthercatMCVelToHomString             "VelToHom"
#define EthercatMCVelFrmHomString            "VelFrmHom"
#define EthercatMCAccHomString               "AccHom"
#define EthercatMCEnc_ActString              "EncAct"
#define EthercatMCErrRstString               "ErrRst"
#define EthercatMCVelActString               "VelAct"
#define EthercatMCVel_RBString               "Vel-RB"
#define EthercatMCAcc_RBString               "Acc-RB"
#define EthercatMCCfgVELO_String             "CfgVELO"
#define EthercatMCCfgVMAX_String             "CfgVMAX"
#define EthercatMCCfgJVEL_String             "CfgJVEL"
#define EthercatMCCfgACCS_String             "CfgACCS"
#define EthercatMCCfgDHLMString              "CfgDHLM"
#define EthercatMCCfgDLLMString              "CfgDLLM"
#define EthercatMCCfgDHLM_EnString           "CfgDHLM-En"
#define EthercatMCCfgDLLM_EnString           "CfgDLLM-En"
#define EthercatMCCfgSREV_RBString           "CfgSREV-RB"
#define EthercatMCCfgUREV_RBString           "CfgUREV-RB"
#define EthercatMCCfgSPDB_RBString           "CfgSPDB-RB"
#define EthercatMCCfgRDBD_RBString           "CfgRDBD-RB"
#define EthercatMCCfgRDBD_Tim_RBString       "CfgRDBD-Tim-RB"
#define EthercatMCCfgRDBD_En_RBString        "CfgRDBD-En-RB"
#define EthercatMCCfgPOSLAG_RBString         "CfgPOSLAG-RB"
#define EthercatMCCfgPOSLAG_Tim_RBString     "CfgPOSLAG-Tim-RB"
#define EthercatMCCfgPOSLAG_En_RBString      "CfgPOSLAG-En-RB"
#define EthercatMCCfgDESC_RBString           "CfgDESC-RB"
#define EthercatMCCfgEGU_RBString            "CfgEGU-RB"


#define EthercatMCMCUErrMsgString            "MCUErrMsg"
#define EthercatMCDbgStrToMcuString          "StrToMCU"

#define HOMPROC_MANUAL_SETPOS    15

extern const char *modNamEMC;

extern "C" {
  unsigned   netToUint(void *data, size_t lenInPlc);
  double     netToDouble(void *data, size_t lenInPlc);
  void       doubleToNet(const double value, void *data, size_t lenInPlc);
  void       uintToNet(const unsigned value, void *data, size_t lenInPlc);
  int EthercatMCCreateAxis(const char *EthercatMCName, int axisNo,
                           int axisFlags, const char *axisOptionsStr);

  asynStatus EthercatMCADSgetPlcMemoryUint(asynUser *pasynUser,
                                           unsigned indexOffset,
                                           unsigned *value,
                                           size_t lenInPlc);
  asynStatus disconnect_C(asynUser *pasynUser);
  asynStatus writeReadOnErrorDisconnect_C(asynUser *pasynUser,
                                          const char *outdata, size_t outlen,
                                          char *indata, size_t inlen);
  asynStatus checkACK(const char *outdata, size_t outlen, const char *indata);
  const char *plcUnitTxtFromUnitCode(unsigned unitCode);
  const char *EthercatMCstrStatus(asynStatus status);
  const char *errStringFromErrId(int nErrorId);
}

class EthercatMCIndexerAxis;

class epicsShareClass EthercatMCController : public asynMotorController {
public:
#define PARAM_IDX_OPMODE_AUTO_UINT32            1
#define PARAM_IDX_MICROSTEPS_UINT32             2
#define PARAM_IDX_ABS_MIN_FLOAT32              30
#define PARAM_IDX_ABS_MAX_FLOAT32              31
#define PARAM_IDX_USR_MIN_FLOAT32              32
#define PARAM_IDX_USR_MAX_FLOAT32              33
#define PARAM_IDX_WRN_MIN_FLOAT32              34
#define PARAM_IDX_WRN_MAX_FLOAT32              35
#define PARAM_IDX_FOLLOWING_ERR_WIN_FLOAT32    55
#define PARAM_IDX_HYTERESIS_FLOAT32            56
#define PARAM_IDX_REFSPEED_FLOAT32             58
#define PARAM_IDX_VBAS_FLOAT32                 59
#define PARAM_IDX_SPEED_FLOAT32                60
#define PARAM_IDX_ACCEL_FLOAT32                61
#define PARAM_IDX_IDLE_CURRENT_FLOAT32         62
#define PARAM_IDX_MOVE_CURRENT_FLOAT32         64
#define PARAM_IDX_MICROSTEPS_FLOAT32           67
#define PARAM_IDX_STEPS_PER_UNIT_FLOAT32       68
#define PARAM_IDX_HOME_POSITION_FLOAT32        69
#define PARAM_IDX_FUN_REFERENCE               133
#define PARAM_IDX_FUN_MOVE_VELOCITY           142


#define FEATURE_BITS_V1               (1)
#define FEATURE_BITS_V2               (1<<1)
#define FEATURE_BITS_V3               (1<<2)
#define FEATURE_BITS_V4               (1<<3)

#define FEATURE_BITS_ADS              (1<<4)
#define FEATURE_BITS_ECMC             (1<<5)
#define FEATURE_BITS_SIM              (1<<6)
#define FEATURE_BITS_GVL              (1<<7)

  EthercatMCController(const char *portName, const char *EthercatMCPortName,
                       int numAxes, double movingPollPeriod,
                       double idlePollPeriod,
                       const char *optionStr);

  void report(FILE *fp, int level);
  asynStatus setMCUErrMsg(const char *value);
  asynStatus configController(int needOk, const char *value);
  asynStatus writeReadOnErrorDisconnect(void);
  EthercatMCAxis* getAxis(asynUser *pasynUser);
  EthercatMCAxis* getAxis(int axisNo);
  int features_;

  protected:
  void handleStatusChange(asynStatus status);
  asynStatus writeReadBinaryOnErrorDisconnect(asynUser *pasynUser,
                                              const char *outdata,
                                              size_t outlen,
                                              char *indata, size_t inlen,
                                              size_t *pnread);
  /* memory bytes via ADS */
  asynStatus writeWriteReadAds(asynUser *pasynUser,
                               AmsHdrType *amsHdr_p, size_t outlen,
                               uint32_t invokeID,
                               uint32_t ads_cmdID,
                               void *indata, size_t inlen,
                               size_t *pnread);
  asynStatus getPlcMemoryViaADS(unsigned indexOffset,
                                void *data, size_t lenInPlc);
  asynStatus setPlcMemoryViaADS(unsigned indexOffset,
                                const void *data, size_t lenInPlc);
  asynStatus getSymbolInfoViaADS(const char *symbolName,
                                 void *data,
                                 size_t lenInPlc);

  /* Indexer */
  asynStatus readDeviceIndexer(unsigned devNum, unsigned infoType);
  void parameterFloatReadBack(unsigned axisNo,
                              unsigned paramIndex,
                              double fValue);
  asynStatus indexerReadAxisParameters(EthercatMCIndexerAxis *pAxis,
                                       unsigned devNum,
                                       unsigned iOffset,
                                       unsigned lenInPlcPara);
  asynStatus poll(void);
  asynStatus newIndexerAxis(EthercatMCIndexerAxis *pAxis,
                            unsigned devNum,
                            unsigned iAllFlags,
                            double   fAbsMin,
                            double   fAbsMax,
                            unsigned iOffset);
  int getFeatures(void);
  asynStatus initialPollIndexer(void);
  asynStatus writeReadControllerPrint(int traceMask);
  asynStatus writeReadACK(int traceMask);
  asynStatus getPlcMemoryUint(unsigned indexOffset,
                              unsigned *value, size_t lenInPlc);
  asynStatus getPlcMemoryString(unsigned indexOffset,
                                char *value, size_t len);
  asynStatus setPlcMemoryInteger(unsigned indexOffset,
                                 int value, size_t lenInPlc);
  asynStatus getPlcMemoryDouble(unsigned indexOffset,
                                double *value, size_t lenInPlc);
  asynStatus setPlcMemoryDouble(unsigned indexOffset,
                                double value, size_t lenInPlc);

  asynStatus indexerParamWaitNotBusy(unsigned indexOffset);
  asynStatus indexerParamRead(unsigned paramIfOffset,
                              unsigned paramIndex,
                              unsigned lenInPlcPara,
                              double *value);
  asynStatus indexerParamWrite(unsigned paramIfOffset,
                               unsigned paramIndex,
                               unsigned lenInPlcPara,
                               double value);

  struct {
    asynStatus   oldStatus;
    unsigned int local_no_ASYN_;
    unsigned int hasConfigError;
    unsigned int initialPollDone;
    unsigned int indexerOffset;
    AmsNetidAndPortType remote;
    AmsNetidAndPortType local;
    unsigned adsport;
    int useADSbinary;
    struct {
      unsigned int stAxisStatus_V1  :1;
      unsigned int stAxisStatus_V2  :1;
      unsigned int bSIM             :1;
      unsigned int bECMC            :1;
      unsigned int bADS             :1;
    } supported;
  } ctrlLocal;


  /* First parameter */
  int EthercatMCErr_;
  int EthercatMCStatusCode_;
  int EthercatMCStatusBits_;
  int EthercatMCaux0_;
  int EthercatMCaux1_;
  int EthercatMCaux2_;
  int EthercatMCaux3_;
  int EthercatMCaux4_;
  int EthercatMCaux5_;
  int EthercatMCaux6_;
  int EthercatMCaux7_;
  int EthercatMCreason8_;
  int EthercatMCreason9_;
  int EthercatMCreason10_;
  int EthercatMCreason11_;
  int EthercatMCHomProc_RB_;
  int EthercatMCHomPos_RB_;
  int EthercatMCHomProc_;
  int EthercatMCHomPos_;
  int EthercatMCVelToHom_;
  int EthercatMCVelFrmHom_;
  int EthercatMCAccHom_;
  int EthercatMCEncAct_;

#ifdef CREATE_MOTOR_REC_RESOLUTION
  int motorRecResolution_;
  int motorRecDirection_;
  int motorRecOffset_;
#endif

  /* Add parameters here */
  int EthercatMCErrRst_;
  int EthercatMCMCUErrMsg_;
  int EthercatMCDbgStrToMcu_;
  int EthercatMCVelAct_;
  int EthercatMCVel_RB_;
  int EthercatMCAcc_RB_;
  int EthercatMCCfgVELO_;
  int EthercatMCCfgVMAX_;
  int EthercatMCCfgJVEL_;
  int EthercatMCCfgACCS_;
  int EthercatMCCfgSREV_RB_;
  int EthercatMCCfgUREV_RB_;
  int EthercatMCCfgSPDB_RB_;
  int EthercatMCCfgRDBD_RB_;
  int EthercatMCCfgRDBD_Tim_RB_;
  int EthercatMCCfgRDBD_En_RB_;
  int EthercatMCCfgPOSLAG_RB_;
  int EthercatMCCfgPOSLAG_Tim_RB_;
  int EthercatMCCfgPOSLAG_En_RB_;
  int EthercatMCCfgDHLM_;
  int EthercatMCCfgDLLM_;
  int EthercatMCCfgDHLM_En_;
  int EthercatMCCfgDLLM_En_;
  int EthercatMCCfgDESC_RB_;
  int EthercatMCCfgEGU_RB_;

  int EthercatMCErrId_;
  /* Last parameter */

  #define FIRST_VIRTUAL_PARAM EthercatMCErr_
  #define LAST_VIRTUAL_PARAM EthercatMCErrId_
  #define NUM_VIRTUAL_MOTOR_PARAMS ((int) (&LAST_VIRTUAL_PARAM - &FIRST_VIRTUAL_PARAM + 1))

  friend class EthercatMCAxis;
  friend class EthercatMCIndexerAxis;
  friend class EthercatMCGvlAxis;
};

#endif
