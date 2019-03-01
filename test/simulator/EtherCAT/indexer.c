/* Implementation of the indexer
  https://forge.frm2.tum.de/public/doc/plc/master/singlehtml/index.html#devices
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "logerr_info.h"
#include "indexer.h"


/* In the memory bytes, the indexer starts at 64 */
static unsigned INDEXEROFFSET;
/*  Device 1 is at here, leaving some space */
#define DEVICE1_OFFSET  100

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
  uint16_t  offset;
  uint16_t  unit;
  uint8_t   flags;
  uint32_t  allFlags;
  uint16_t  parameters[16]; /* counting 0..15 */
  char      name[34];
  char      aux[8][34];
  float     absMin;
  float     absMax;
} indexerDeviceAbsStraction_type;

indexerDeviceAbsStraction_type indexerDeviceAbsStraction[2] =
{
  /* device 0, the indexer itself */
  { 0, 38,  0 /* INDEXEROFFSET */, 0, 0,
    0,
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    "indexer",
    { "", "", "", "", "", "", "", "" },
    0.1, 0.2
  },
  { 0x5008, 0x8,  DEVICE1_OFFSET, 0xc04, 0,
    0,
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    "motor1",
    { "", "", "", "", "", "", "", "" },
    5.0, 175.0
  }
};




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
    } memoryStruct;
} idxData;

static int initDone = 0;

static void init(void)
{
  if (initDone) return;
  memset (&idxData, 0, sizeof(idxData));
  idxData.memoryStruct.magic = 2015.02;
  idxData.memoryStruct.offset = INDEXEROFFSET;
  INDEXEROFFSET =
    (unsigned)((void*)&idxData.memoryStruct.indexer_ack - (void*)&idxData);
  idxData.memoryStruct.offset = INDEXEROFFSET;

  LOGINFO3("%s/%s:%d INDEXEROFFSET=%u\n",
           __FILE__, __FUNCTION__, __LINE__,
           INDEXEROFFSET);

  initDone = 1;
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
      idxData.memoryStruct.indexer.infoType0.offset = 100 + 16 * devNum; /* TODO */
      idxData.memoryStruct.indexer.infoType0.unit = indexerDeviceAbsStraction[devNum].unit;
      /* TODO: calc the flags from lenght of AUX strings */
      //idxData.memoryStruct.indexer.infoType0.flagsLow = indexerDeviceAbsStraction[devNum].flagsLow;
      //idxData.memoryStruct.indexer.infoType0.flagsHigh = indexerDeviceAbsStraction[devNum].flagsHigh;
      idxData.memoryStruct.indexer.infoType0.absMin = indexerDeviceAbsStraction[devNum].absMin;
      idxData.memoryStruct.indexer.infoType0.absMax = indexerDeviceAbsStraction[devNum].absMax;
      if (!devNum) {
        /* The indexer himself. */
        idxData.memoryStruct.indexer.infoType0.offset = INDEXEROFFSET;
        idxData.memoryStruct.indexer.infoType0.flagsHigh = 0x8000; /* extended indexer */
      }
      LOGINFO3("%s/%s:%d idxData=%p indexer=%p delta=%u typeCode=%u size=%u offset=%u ack=0x%x\n",
               __FILE__, __FUNCTION__, __LINE__,
               &idxData, &idxData.memoryStruct.indexer,
               (unsigned)((void*)&idxData.memoryStruct.indexer - (void*)&idxData),
               idxData.memoryStruct.indexer.infoType0.typeCode,
               idxData.memoryStruct.indexer.infoType0.size,
               idxData.memoryStruct.indexer.infoType0.offset,
               idxData.memoryStruct.indexer_ack);

      idxData.memoryStruct.indexer_ack |= 0x8000; /* ACK */
      return 0;
    case 4:
      /* get values from device table */
      strncpy(&idxData.memoryStruct.indexer.infoType4.name[0],
              indexerDeviceAbsStraction[devNum].name,
              sizeof(idxData.memoryStruct.indexer.infoType4.name));
      LOGINFO3("%s/%s:%d devName=%s idxName=%s\n",
               __FILE__, __FUNCTION__, __LINE__,
               indexerDeviceAbsStraction[devNum].name,
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
    ret = idxData.memoryBytes[indexOffset] +
      (idxData.memoryBytes[indexOffset + 1] << 8);
    *uValue = ret;
    LOGINFO3("%s/%s:%d adsport=%u indexOffset=%u len_in_PLC=%u ret=%u (0x%x)\n",
             __FILE__, __FUNCTION__, __LINE__,
             adsport,
             indexOffset,
             len_in_PLC,
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
  if (indexOffset == INDEXEROFFSET) {
    return indexerHandleIndexerCmd(indexOffset, len_in_PLC, uValue);
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
