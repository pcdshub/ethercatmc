/*
FILENAME...   EthercatMC.h
*/


#ifdef AXISNOTMOTOR
#include "asynAxisController.h"
#include "asynAxisAxis.h"
#else
#include "asynMotorController.h"
#include "asynMotorAxis.h"
#endif




#define AMPLIFIER_ON_FLAG_CREATE_AXIS  (1)
#define AMPLIFIER_ON_FLAG_WHEN_HOMING  (1<<1)
#define AMPLIFIER_ON_FLAG_USING_CNEN   (1<<2)

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

extern const char *modNamEMC;

extern "C" {
  int EthercatMCCreateAxis(const char *EthercatMCName, int axisNo,
                      int axisFlags, const char *axisOptionsStr);
  asynStatus writeReadOnErrorDisconnect_C(asynUser *pasynUser,
                                          const char *outdata, size_t outlen,
                                          char *indata, size_t inlen);
  asynStatus checkACK(const char *outdata, size_t outlen, const char *indata);
  const char *plcUnitTxtFromUnitCode(unsigned unitCode);
}

typedef struct {
  /* V1 members */
  int bEnable;           /*  1 */
  int bReset;            /*  2 */
  int bExecute;          /*  3 */
  int nCommand;          /*  4 */
  int nCmdData;          /*  5 */
  double fVelocity;      /*  6 */
  double fPosition;      /*  7 */
  double fAcceleration;  /*  8 */
  double fDecceleration; /*  9 */
  int bJogFwd;           /* 10 */
  int bJogBwd;           /* 11 */
  int bLimitFwd;         /* 12 */
  int bLimitBwd;         /* 13 */
  double fOverride;      /* 14 */
  int bHomeSensor;       /* 15 */
  int bEnabled;          /* 16 */
  int bError;            /* 17 */
  int nErrorId;          /* 18 */
  double fActVelocity;   /* 19 */
  double fActPosition;   /* 20 */
  double fActDiff;       /* 21 */
  int bHomed;            /* 22 */
  int bBusy;             /* 23 */
  /* V2 members */
  double encoderRaw;
  int atTarget;
  /* neither V1 nor V2, but calculated here */
  int mvnNRdyNex; /* Not in struct. Calculated in poll() */
  int motorStatusDirection; /* Not in struct. Calculated in pollAll() */
  int motorDiffPostion;     /* Not in struct. Calculated in poll() */
} st_axis_status_type;

class epicsShareClass EthercatMCAxis : public asynMotorAxis
{
public:
  /* These are the methods we override from the base class */
  EthercatMCAxis(class EthercatMCController *pC, int axisNo,
            int axisFlags, const char *axisOptionsStr);
  void report(FILE *fp, int level);
  asynStatus mov2(double posEGU, int nCommand, double maxVeloEGU, double accEGU);
  asynStatus move(double position, int relative, double min_velocity, double max_velocity, double acceleration);
  asynStatus moveVelocity(double min_velocity, double max_velocity, double acceleration);
  asynStatus setPosition(double);

  asynStatus home(double min_velocity, double max_velocity, double acceleration, int forwards);
  asynStatus stop(double acceleration);
  void       callParamCallbacksUpdateError();
  asynStatus pollAll(bool *moving);
  asynStatus pollAll(bool *moving, st_axis_status_type *pst_axis_status);
  asynStatus poll(bool *moving);

private:
  typedef enum
  {
    eeAxisErrorIOCcomError = -1,
    eeAxisErrorNoError,
    eeAxisErrorMCUError,
    eeAxisErrorCmdError,
    eeAxisErrorNotFound,
    eeAxisErrorNotHomed,
    eeAxisIllegalInTargetWindow
  } eeAxisErrorType;

  typedef enum
  {
    eeAxisWarningNoWarning,
    eeAxisWarningCfgZero,
    eeAxisWarningVeloZero,
    eeAxisWarningSpeedLimit
  } eeAxisWarningType;

  typedef enum
  {
    pollNowReadScaling,
    pollNowReadMonitoring,
    pollNowReadBackSoftLimits,
    pollNowReadBackVelocities
  } eeAxisPollNowType;

  EthercatMCController *pC_;          /**< Pointer to the asynMotorController to which this axis belongs.
                                   *   Abbreviated because it is used very frequently */
  struct {
    st_axis_status_type old_st_axis_status;
    double scaleFactor;
    double eres;
    const char *externalEncoderStr;
    const char *cfgfileStr;
    const char *cfgDebug_str;
    int axisFlags;
    int MCU_nErrorId;     /* nErrorID from MCU */
    int old_MCU_nErrorId; /* old nErrorID from MCU */
    int old_EPICS_nErrorId; /* old nErrorID from MCU */

    int old_bError;   /* copy of bError */
#ifndef motorWaitPollsBeforeReadyString
    unsigned int waitNumPollsBeforeReady;
#endif
    int nCommandActive;
    int old_nCommandActive;
    int homed;
    unsigned int illegalInTargetWindow :1;
    eeAxisWarningType old_eeAxisWarning;
    eeAxisWarningType eeAxisWarning;
    eeAxisErrorType old_eeAxisError;
    eeAxisErrorType eeAxisError;
    eeAxisPollNowType eeAxisPollNow;
    /* Which values have changed in the EPICS IOC, but are not updated in the
       motion controller */
    struct {
      int          nMotionAxisID;     /* Needed for ADR commands */
      unsigned int stAxisStatus_Vxx :1;
      unsigned int statusVer        :1;
      unsigned int oldStatusDisconnected : 1;
      unsigned int sErrorMessage    :1; /* From MCU */
      unsigned int initialPollNeeded :1;
    }  dirty;

    struct {
      unsigned int stAxisStatus_V1  :1;
      unsigned int stAxisStatus_V2  :1;
      unsigned int bV1BusyNewStyle  :1;
      unsigned int bSIM             :1;
      unsigned int bECMC            :1;
      unsigned int bADS             :1;
      int          statusVer;           /* 0==V1, busy old style 1==V1, new style*/
                                        /* 2==V2 */
    }  supported;
    /* Error texts when we talk to the controller, there is not an "OK"
       Or, failure in setValueOnAxisVerify() */
    char cmdErrorMessage[80]; /* From driver */
    char sErrorMessage[80]; /* From controller */
    char adsport_str[15]; /* "ADSPORT=12345/" */ /* 14 should be enough, */
    char adsport_zero[1]; /* 15 + 1 for '\' keep us aligned in memory */
    unsigned int adsPort;
  } drvlocal;

  asynStatus handleConnect(void);
  asynStatus writeReadControllerPrint(int traceMask);
  asynStatus writeReadControllerPrint(void);
  asynStatus readConfigLine(const char *line, const char **errorTxt_p);
  asynStatus readConfigFile(void);
  asynStatus readBackAllConfig(int axisID);
  asynStatus readBackSoftLimits(void);
  asynStatus readBackHoming(void);
  asynStatus readScaling(int axisID);
  asynStatus readMonitoring(int axisID);
  asynStatus readBackVelocities(int axisID);
  asynStatus initialPoll(void);
  asynStatus initialPollInternal(void);
  asynStatus setValueOnAxis(const char* var, int value);
  asynStatus setValueOnAxisVerify(const char *var, const char *rbvar,
                                  int value, unsigned int retryCount);
  asynStatus setValueOnAxis(const char* var, double value);
  asynStatus setValuesOnAxis(const char* var1, double value1, const char* var2, double value2);
  int getMotionAxisID(void);
  asynStatus getFeatures(void);
  asynStatus setSAFValueOnAxis(unsigned indexGroup,
                               unsigned indexOffset,
                               int value);

  asynStatus setSAFValueOnAxisVerify(unsigned indexGroup,
                                     unsigned indexOffset,
                                     int value,
                                     unsigned int retryCount);

  asynStatus setSAFValueOnAxis(unsigned indexGroup,
                               unsigned indexOffset,
                               double value);

  asynStatus setSAFValueOnAxisVerify(unsigned indexGroup,
                                     unsigned indexOffset,
                                     double value,
                                     unsigned int retryCount);

  asynStatus getSAFValueFromAxisPrint(unsigned indexGroup,
                                      unsigned indexOffset,
                                      const char *name,
                                      int *value);

  asynStatus getSAFValuesFromAxisPrint(unsigned iIndexGroup,
                                       unsigned iIndexOffset,
                                       const char *iname,
                                       int *iValue,
                                       unsigned fIndexGroup,
                                       unsigned fIndexOffset,
                                       const char *fname,
                                       double *fValue);

  asynStatus getSAFValueFromAxisPrint(unsigned indexGroup,
                                      unsigned indexOffset,
                                      const char *name,
                                      double *value);

  asynStatus getValueFromAxis(const char* var, int *value);
  asynStatus getValueFromAxis(const char* var, double *value);
  asynStatus getStringFromAxis(const char* var, char *value, size_t maxlen);
  asynStatus getValueFromController(const char* var, double *value);

  asynStatus resetAxis(void);
  bool       pollPowerIsOn(void);
  asynStatus enableAmplifier(int);
  asynStatus sendVelocityAndAccelExecute(double maxVeloEGU, double accEGU);
  asynStatus setClosedLoop(bool closedLoop);
  asynStatus setIntegerParam(int function, int value);
  asynStatus setDoubleParam(int function, double value);
  asynStatus setStringParamDbgStrToMcu(const char *value);
  asynStatus setStringParam(int function, const char *value);
  asynStatus stopAxisInternal(const char *function_name, double acceleration);
#ifndef motorMessageTextString
  void updateMsgTxtFromDriver(const char *value);
#endif

  friend class EthercatMCController;
};

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

  EthercatMCController(const char *portName, const char *EthercatMCPortName, int numAxes, double movingPollPeriod, double idlePollPeriod);

  void report(FILE *fp, int level);
  asynStatus setMCUErrMsg(const char *value);
  asynStatus configController(int needOk, const char *value);
  asynStatus writeReadOnErrorDisconnect(void);
  EthercatMCAxis* getAxis(asynUser *pasynUser);
  EthercatMCAxis* getAxis(int axisNo);
  protected:
  void handleStatusChange(asynStatus status);
  /* Indexer */
  asynStatus readDeviceIndexer(unsigned devNum, unsigned infoType);
  void parameterFloatReadBack(unsigned axisNo,
                              unsigned paramIndex,
                              double fValue);
  asynStatus indexerReadAxisParameters(EthercatMCIndexerAxis *pAxis,
                                       unsigned devNum, unsigned iOffset);
  asynStatus poll(void);
  asynStatus newIndexerAxis(EthercatMCIndexerAxis *pAxis,
                            unsigned devNum,
                            unsigned iAllFlags,
                            double   fAbsMin,
                            double   fAbsMax,
                            unsigned iOffset);
  asynStatus initialPollIndexer(void);
  asynStatus writeReadControllerPrint(int traceMask);
  asynStatus writeReadACK(int traceMask);
  asynStatus getPlcMemoryUint(unsigned indexOffset,
                              unsigned *value, size_t lenInPlc);
  asynStatus getPlcMemorySint(unsigned indexOffset,
                              int *value, size_t lenInPlc);
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
  asynStatus indexerParamWrite(unsigned paramIfOffset, unsigned paramIndex,
                               double value);

  struct {
    unsigned int local_no_ASYN_;
    unsigned int hasConfigError;
    unsigned int isConnected;
    unsigned int initialPollDone;
    unsigned int indexerOffset;
  } ctrlLocal;

  unsigned adsport;

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
};
