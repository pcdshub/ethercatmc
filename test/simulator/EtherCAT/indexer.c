#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "logerr_info.h"
#include "indexer.h"


#define INDEXEROFFSET    62
#define DEVICE0_OFFSET  100

typedef struct {
  uint16_t  typeCode;
  uint16_t  size;
  uint16_t  offset;
  uint8_t   flags;
  uint32_t  allFlags;
  uint16_t  parameters[16]; /* counting 0..15 */
  char      name[34];
  char      aux[8][34];
  float     absMin;
  float     absMax;
} indexerDeviceAbsStraction_type;

indexerDeviceAbsStraction_type indexerDeviceAbsStraction[1] =
  {
    { 0x5008, 0x08,  DEVICE0_OFFSET, 0,
      0,
      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
      "motor1",
      { "", "", "", "", "", "", "", "" },
      5.0, 175.0 }
  };




static union {
  uint8_t memoryBytes[1024];
  struct {
    float    magic;
    uint16_t offset;
  } memoryStruct;
} idxData;

static int initDone = 0;

static void init(void)
{
  if (initDone) return;
  memset (&idxData, 0, sizeof(idxData));
  idxData.memoryStruct.magic = 2015.02;
  idxData.memoryStruct.offset = INDEXEROFFSET;

  initDone = 1;
}

static int indexerHandleIndexerCmd(unsigned indexOffset,
                                   unsigned len_in_PLC,
                                   unsigned uValue)
{
  unsigned devNum = uValue & 0xFF;
  unsigned infoType = (uValue >> 8) & 0xFF;
  LOGINFO3("%s/%s:%d indexOffset=%u len_in_PLC=%u uValue=%u devNum=%u infoType=%u\n",
           __FILE__, __FUNCTION__, __LINE__,
           indexOffset, len_in_PLC,
           uValue, devNum, infoType);
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
  switch(indexOffset) {
    case INDEXEROFFSET:
      return indexerHandleIndexerCmd(indexOffset, len_in_PLC, uValue);
    default:
      break;
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


