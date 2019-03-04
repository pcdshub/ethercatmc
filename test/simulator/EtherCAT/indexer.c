/* Implementation of the indexer
  https://forge.frm2.tum.de/public/doc/plc/master/singlehtml/index.html#devices
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "hw_motor.h"
#include "indexer.h"
#include "logerr_info.h"

/* type codes and sizes */
#define TYPECODE_INDEXER               0
#define SIZE_INDEXER                  38
#define TYPECODE_PARAMDEVICE_5008 0x5008
#define SIZE_PARAMDEVICE_5008        0x8


/* Well known unit codes */
#define UNITCODE_NONE                    0
#define UNITCODE_MM                 0xfc04

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


/* In the memory bytes, the indexer starts at 64 */
static unsigned offsetIndexer;

/* Info types of the indexer */
typedef struct {
    uint16_t   typeCode;
    uint16_t   size;
    uint16_t   offset;
    uint16_t   unit;
    uint16_t   flagsLow;
    uint16_t   flagsHigh;
    float      absMin;
    float      absMax;
} indexerInfoType0_type;

typedef struct {
  char name[33]; /* leave one byte for trailing '\0' */
} indexerInfoType4_type;



typedef struct {
  uint16_t  typeCode;
  uint16_t  size;
  uint16_t  unitCode;
  uint16_t  parameters[16]; /* counting 0..15 */
  char      devName[34];
  char      auxName[8][34];
  float     absMin;
  float     absMax;
} indexerDeviceAbsStraction_type;


/* The paramDevice structure.
   floating point values are 4 bytes long,
   the whole structure uses 16 bytes, 8 words */
typedef struct {
  float     actualValue;
  float     targetValue;
  uint16_t  statusReasonAux;
  uint16_t  paramCtrl;
  float     paramValue;
} indexerDevice5008interface_type;


indexerDeviceAbsStraction_type indexerDeviceAbsStraction[2] =
{
  /* device 0, the indexer itself */
  { TYPECODE_INDEXER, SIZE_INDEXER,
    UNITCODE_NONE,
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    "indexer",
    { "", "", "", "", "", "", "", "" },
    0.0, 0.0
  },
  { TYPECODE_PARAMDEVICE_5008, SIZE_PARAMDEVICE_5008,
    UNITCODE_MM,
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    "SimAxis1",
    { "", "", "", "", "", "homing", "@home", "homed" },
    5.0, 175.0
  }
};


typedef struct
{
  double fVelocity;
  double fAcceleration;
  double fDeceleration;
} cmd_Motor_cmd_type;


static union {
  uint8_t  memoryBytes[1024];
  uint16_t memoryWords[512];
  struct {
    float    magic;
    uint16_t offset;
    uint16_t indexer_ack;
    /* Area for the indexer. union of the different info types */
    union {
      indexerInfoType0_type infoType0;
      indexerInfoType4_type infoType4;
      } indexer;
    indexerDevice5008interface_type motor1;
    } memoryStruct;
} idxData;

static int initDone = 0;
static unsigned offsetMotor1StatusReasonAux;

/* values commanded to the motor */
static cmd_Motor_cmd_type cmd_Motor_cmd[MAX_AXES];


static void init(void)
{
  if (initDone) return;
  memset (&idxData, 0, sizeof(idxData));
  idxData.memoryStruct.magic = 2015.02;
  idxData.memoryStruct.offset = offsetIndexer;
  offsetIndexer =
    (unsigned)((void*)&idxData.memoryStruct.indexer_ack - (void*)&idxData);
  idxData.memoryStruct.offset = offsetIndexer;

  LOGINFO3("%s/%s:%d offsetIndexer=%u\n",
           __FILE__, __FUNCTION__, __LINE__,
           offsetIndexer);

  offsetMotor1StatusReasonAux =
    (unsigned)((void*)&idxData.memoryStruct.motor1.statusReasonAux - (void*)&idxData);
  initDone = 1;
}

static void init_axis(int axis_no)
{
  static char init_done[MAX_AXES];
  const double MRES = 1;
  const double UREV = 60.0; /* mm/revolution */
  const double SREV = 2000.0; /* ticks/revolution */
  const double ERES = UREV / SREV;

  double ReverseMRES = (double)1.0/MRES;

  if (axis_no >= MAX_AXES || axis_no < 0) {
    return;
  }
  if (!init_done[axis_no]) {
    struct motor_init_values motor_init_values;
    double valueLow = -1.0 * ReverseMRES;
    double valueHigh = 186.0 * ReverseMRES;
    memset(&motor_init_values, 0, sizeof(motor_init_values));
    motor_init_values.ReverseERES = MRES/ERES;
    motor_init_values.ParkingPos = (100 + axis_no/10.0);
    motor_init_values.MaxHomeVelocityAbs = 5 * ReverseMRES;
    motor_init_values.lowHardLimitPos = valueLow;
    motor_init_values.highHardLimitPos = valueHigh;
    motor_init_values.hWlowPos = valueLow;
    motor_init_values.hWhighPos = valueHigh;

    hw_motor_init(axis_no,
                  &motor_init_values,
                  sizeof(motor_init_values));

    //cmd_Motor_cmd[axis_no].maximumVelocity = 50;
    //
    //cmd_Motor_cmd[axis_no].homeVeloTowardsHomeSensor = 10;
    //cmd_Motor_cmd[axis_no].homeVeloFromHomeSensor = 5;
    //cmd_Motor_cmd[axis_no].fPosition = getMotorPos(axis_no);
    //cmd_Motor_cmd[axis_no].referenceVelocity = 600;
    //cmd_Motor_cmd[axis_no].inTargetPositionMonitorWindow = 0.1;
    //cmd_Motor_cmd[axis_no].inTargetPositionMonitorTime = 0.02;
    //cmd_Motor_cmd[axis_no].inTargetPositionMonitorEnabled = 1;
    setMRES_23(axis_no, UREV);
    setMRES_24(axis_no, SREV);

    init_done[axis_no] = 1;
  }
}

static unsigned
indexerMotorStatusRead(unsigned motor_axis_no,
                       indexerDevice5008interface_type *pIndexerDevice5008interface)
{
  unsigned ret = 0;
  unsigned statusReasonAux;
  idxStatusCodeType idxStatusCode;
  /* The following only works on little endian (?)*/
  statusReasonAux = pIndexerDevice5008interface->statusReasonAux;
  idxStatusCode = (idxStatusCodeType)(statusReasonAux >> 12);
  /* The following would be run in an own task in a PLC program.
     For the simulator, we hook the code into the read request
     RESET, START and STOP are commands from IOC.
     RESET is even the "wakeup" state.
  */

  switch (idxStatusCode) {
  case idxStatusCodeRESET:
    init_axis((int)motor_axis_no);
    motorStop(motor_axis_no);
    set_nErrorId(motor_axis_no, 0);
    break;
  case idxStatusCodeSTART:
    movePosition(motor_axis_no,
                 pIndexerDevice5008interface->targetValue,
                 0, /* int relative, */
                 cmd_Motor_cmd[motor_axis_no].fVelocity,
                 cmd_Motor_cmd[motor_axis_no].fAcceleration);
    break;
  case idxStatusCodeSTOP:
    motorStop(motor_axis_no);
    break;
  default:
    ;
  }
  statusReasonAux = 0;

  /* reason bits */
  if (getPosLimitSwitch(motor_axis_no))
    statusReasonAux |= 0x0800;
  if (getNegLimitSwitch(motor_axis_no))
    statusReasonAux |= 0x0400;

  /* the status bits */
  if (get_bError(motor_axis_no))
    idxStatusCode = idxStatusCodeERROR;
  else if (!getAmplifierOn(motor_axis_no))
    idxStatusCode = idxStatusCodePOWEROFF;
  else if (isMotorMoving(motor_axis_no))
    idxStatusCode = idxStatusCodeBUSY;
  else if(statusReasonAux)
    idxStatusCode = idxStatusCodeWARN;
  else
    idxStatusCode = idxStatusCodeIDLE;

  ret = statusReasonAux | (idxStatusCode << 12);;
  pIndexerDevice5008interface->statusReasonAux = ret;
  return ret;
}





static int indexerHandleIndexerCmd(unsigned indexOffset,
                                   unsigned len_in_PLC,
                                   unsigned uValue)
{
  unsigned devNum = uValue & 0xFF;
  unsigned infoType = (uValue >> 8) & 0x7F;
  LOGINFO3("%s/%s:%d indexOffset=%u len_in_PLC=%u uValue=%u devNum=%u infoType=%u\n",
           __FILE__, __FUNCTION__, __LINE__,
           indexOffset, len_in_PLC,
           uValue, devNum, infoType);
  memset(&idxData.memoryStruct.indexer, 0, sizeof(idxData.memoryStruct.indexer));
  idxData.memoryStruct.indexer_ack = uValue;
  if (devNum >= (sizeof(indexerDeviceAbsStraction)/
                 sizeof(indexerDeviceAbsStraction[0]))) {
    idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
    return 0;
  }
  switch (infoType) {
    case 0:
      /* get values from device table */
      idxData.memoryStruct.indexer.infoType0.typeCode = indexerDeviceAbsStraction[devNum].typeCode;
      idxData.memoryStruct.indexer.infoType0.size = indexerDeviceAbsStraction[devNum].size;
      idxData.memoryStruct.indexer.infoType0.unit = indexerDeviceAbsStraction[devNum].unitCode;
      /* TODO: calc the flags from lenght of AUX strings */
      //idxData.memoryStruct.indexer.infoType0.flagsLow = indexerDeviceAbsStraction[devNum].flagsLow;
      //idxData.memoryStruct.indexer.infoType0.flagsHigh = indexerDeviceAbsStraction[devNum].flagsHigh;
      idxData.memoryStruct.indexer.infoType0.absMin = indexerDeviceAbsStraction[devNum].absMin;
      idxData.memoryStruct.indexer.infoType0.absMax = indexerDeviceAbsStraction[devNum].absMax;
      if (!devNum) {
        /* The indexer himself. */
        idxData.memoryStruct.indexer.infoType0.offset = offsetIndexer;
        idxData.memoryStruct.indexer.infoType0.flagsHigh = 0x8000; /* extended indexer */
      } else {
        unsigned auxIdx;
        unsigned flagsLow = 0;
        unsigned maxAuxIdx;
        unsigned offset;
        maxAuxIdx = sizeof(indexerDeviceAbsStraction[devNum].auxName) /
          sizeof(indexerDeviceAbsStraction[devNum].auxName[0]);

        for (auxIdx = maxAuxIdx; auxIdx; auxIdx--) {
          if (strlen(indexerDeviceAbsStraction[devNum].auxName[auxIdx])) {
            flagsLow |= 1;
          }
          flagsLow = flagsLow << 1;
          LOGINFO3("%s/%s:%d auxIdx=%u flagsLow=0x%x\n",
                   __FILE__, __FUNCTION__, __LINE__,
                   auxIdx, flagsLow);
        }
        idxData.memoryStruct.indexer.infoType0.flagsLow = flagsLow;
        /* Offset to the first motor */
        offset = (unsigned)((void*)&idxData.memoryStruct.motor1 - (void*)&idxData);
        if (devNum > 1) {
          /* TODO: Support other interface types */
          offset += sizeof(indexerDevice5008interface_type);
        }

        idxData.memoryStruct.indexer.infoType0.offset = offset;
      }
      LOGINFO3("%s/%s:%d idxData=%p indexer=%p delta=%u typeCode=%u size=%u offset=%u flagsLow=0x%xack=0x%x\n",
               __FILE__, __FUNCTION__, __LINE__,
               &idxData, &idxData.memoryStruct.indexer,
               (unsigned)((void*)&idxData.memoryStruct.indexer - (void*)&idxData),
               idxData.memoryStruct.indexer.infoType0.typeCode,
               idxData.memoryStruct.indexer.infoType0.size,
               idxData.memoryStruct.indexer.infoType0.offset,
               idxData.memoryStruct.indexer.infoType0.flagsLow,
               idxData.memoryStruct.indexer_ack);

      idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
      return 0;
    case 4:
      /* get values from device table */
      strncpy(&idxData.memoryStruct.indexer.infoType4.name[0],
              indexerDeviceAbsStraction[devNum].devName,
              sizeof(idxData.memoryStruct.indexer.infoType4.name));
      LOGINFO3("%s/%s:%d devName=%s idxName=%s\n",
               __FILE__, __FUNCTION__, __LINE__,
               indexerDeviceAbsStraction[devNum].devName,
               &idxData.memoryStruct.indexer.infoType4.name[0]);
      idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
      return 0;
    case 5: /* version */
    case 6: /* author 1 */
    case 7: /* author 2 */
      idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
      return 0;
    case 15:
      /* TODO: parameter */
      idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
      return 0;
    default:
      if (infoType >= 16 && infoType <= 23) {
        /* Support for aux bits 7..0
           Bits 23..16 are not (yet) supported */
        strncpy(&idxData.memoryStruct.indexer.infoType4.name[0],
                indexerDeviceAbsStraction[devNum].auxName[infoType-16],
                sizeof(idxData.memoryStruct.indexer.infoType4.name));
        idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
        return 0;
      }
      return __LINE__;
    }
  return __LINE__;
}

/*************************************************************************/
int indexerHandleADS_ADR_getUInt(unsigned adsport,
                                 unsigned indexOffset,
                                 unsigned len_in_PLC,
                                 unsigned *uValue)
{
  unsigned ret;
  init();
  if (indexOffset + len_in_PLC >= sizeof(idxData))
    return __LINE__;
  if (len_in_PLC == 2) {
    if (indexOffset == offsetMotor1StatusReasonAux) {
      unsigned motor = 1;
      ret = indexerMotorStatusRead(motor, &idxData.memoryStruct.motor1);
      *uValue = ret;
      return 0;
    }
    ret = idxData.memoryBytes[indexOffset] +
      (idxData.memoryBytes[indexOffset + 1] << 8);
    *uValue = ret;
    LOGINFO3("%s/%s:%d adsport=%u indexOffset=%u len_in_PLC=%u mot1=%u ret=%u (0x%x)\n",
             __FILE__, __FUNCTION__, __LINE__,
             adsport,
             indexOffset,
             len_in_PLC,
             offsetMotor1StatusReasonAux,
             ret, ret);

    return 0;
  } else if (len_in_PLC == 4) {
    ret = idxData.memoryBytes[indexOffset] +
      (idxData.memoryBytes[indexOffset + 1] << 8) +
      (idxData.memoryBytes[indexOffset + 2] << 16) +
      (idxData.memoryBytes[indexOffset + 3] << 24);
    *uValue = ret;
    return 0;
  }
  return __LINE__;
}

int indexerHandleADS_ADR_putUInt(unsigned adsport,
                                 unsigned indexOffset,
                                 unsigned len_in_PLC,
                                 unsigned uValue)
{
  init();
  LOGINFO3("%s/%s:%d adsport=%u indexOffset=%u len_in_PLC=%u uValue=%u\n",
           __FILE__, __FUNCTION__, __LINE__,
           adsport,
           indexOffset,
           len_in_PLC,
           uValue);
  if (indexOffset == offsetIndexer) {
    return indexerHandleIndexerCmd(indexOffset, len_in_PLC, uValue);
  } else if (indexOffset == offsetMotor1StatusReasonAux) {
    indexerDevice5008interface_type *pIndexerDevice5008interface;
    pIndexerDevice5008interface = &idxData.memoryStruct.motor1;
    pIndexerDevice5008interface->statusReasonAux = uValue;
    return 0;
  }

  return __LINE__;
}

int indexerHandleADS_ADR_getFloat(unsigned adsport,
                                  unsigned indexOffset,
                                  unsigned len_in_PLC,
                                  double *fValue)
{
  float fRet;
  init();
  if (indexOffset + len_in_PLC >= sizeof(idxData))
    return 1;
  if (len_in_PLC == 4) {
    memcpy(&fRet,
           &idxData.memoryBytes[indexOffset],
           sizeof(fRet));
    *fValue = (double)fRet;
    return 0;
  }
  return __LINE__;
}


int indexerHandleADS_ADR_putFloat(unsigned adsport,
                                  unsigned indexOffset,
                                  unsigned len_in_PLC,
                                  double fValue)
{
  init();
  return 0;
};


int indexerHandleADS_ADR_getString(unsigned adsport,
                                   unsigned indexOffset,
                                   unsigned len_in_PLC,
                                   char **sValue)
{
  init();
  *sValue = (char *)&idxData.memoryBytes[indexOffset];
  return 0;
};
