#ifndef AMS_H
#define AMS_H

#include <inttypes.h>
#include <stddef.h>

#define ADS_READ_DEVICE_INFO  1
#define ADS_READ              2
#define ADS_WRITE             3
#define ADS_READ_WRITE        9


/* AMS/TCP Header */
typedef struct {
  uint8_t res0;
  uint8_t res1;
  uint8_t net_len[4];
} AmsTcpHdrType;

typedef struct AmsNetidAndPortType {
  uint8_t netID[6];
  uint8_t port_low;
  uint8_t port_high;
} AmsNetidAndPortType;



typedef struct {
  AmsTcpHdrType ams_tcp_hdr;
  AmsNetidAndPortType target;
  AmsNetidAndPortType source;
  uint8_t cmdID_low;
  uint8_t cmdID_high;
  uint8_t stateFlags_low;
  uint8_t stateFlags_high;
  uint8_t net_len[4];
  uint8_t errorCode_0;
  uint8_t errorCode_1;
  uint8_t errorCode_2;
  uint8_t errorCode_3;
  uint8_t invokeID_0;
  uint8_t invokeID_1;
  uint8_t invokeID_2;
  uint8_t invokeID_3;
} AmsHdrType;

typedef struct {
  AmsHdrType ams_hdr;
  uint8_t indexGroup_0;
  uint8_t indexGroup_1;
  uint8_t indexGroup_2;
  uint8_t indexGroup_3;
  uint8_t indexOffset_0;
  uint8_t indexOffset_1;
  uint8_t indexOffset_2;
  uint8_t indexOffset_3;
  uint8_t net_len[4];
} AdsReadReqType;

typedef struct {
  AmsHdrType ams_hdr;
  uint8_t indexGroup_0;
  uint8_t indexGroup_1;
  uint8_t indexGroup_2;
  uint8_t indexGroup_3;
  uint8_t indexOffset_0;
  uint8_t indexOffset_1;
  uint8_t indexOffset_2;
  uint8_t indexOffset_3;
  uint8_t net_len[4];
} AdsWriteReqType;

typedef struct {
  AmsHdrType ams_hdr;
  uint8_t indexGroup_0;
  uint8_t indexGroup_1;
  uint8_t indexGroup_2;
  uint8_t indexGroup_3;
  uint8_t indexOffset_0;
  uint8_t indexOffset_1;
  uint8_t indexOffset_2;
  uint8_t indexOffset_3;
  uint8_t rd_len_0;
  uint8_t rd_len_1;
  uint8_t rd_len_2;
  uint8_t rd_len_3;
  uint8_t wr_len_0;
  uint8_t wr_len_1;
  uint8_t wr_len_2;
  uint8_t wr_len_3;
} AdsReadWriteReqType;

typedef struct {
  AmsHdrType ams_hdr;
  struct {
    uint8_t result_0;
    uint8_t result_1;
    uint8_t result_2;
    uint8_t result_3;
    uint8_t major;
    uint8_t minor;
    uint8_t versionBuild_low;
    uint8_t versionBuild_high;
    char    deviceName[16];
  } response;
} AdsReadDeviceInfoRepType;

typedef struct {
  AmsHdrType ams_hdr;
  struct {
    uint8_t result_0;
    uint8_t result_1;
    uint8_t result_2;
    uint8_t result_3;
    uint8_t net_len[4];
  } response;
} AdsReadRepType;

typedef struct {
  AmsHdrType ams_hdr;
  struct {
    uint8_t result_0;
    uint8_t result_1;
    uint8_t result_2;
    uint8_t result_3;
    uint8_t net_len[4];
  } response;
} AdsReadWriteRepType;


typedef struct {
  AmsHdrType ams_hdr;
  struct {
    uint8_t result_0;
    uint8_t result_1;
    uint8_t result_2;
    uint8_t result_3;
  } response;
} AdsWriteRepType;


typedef struct {
  AdsReadWriteRepType ads_read_write_rep;
  struct {
    uint8_t entryLen[4];
    uint8_t indexGroup[4];
    uint8_t indexOffset[4];
    uint8_t size[4];
    uint8_t dataType[4];
    uint8_t flags[4];
    uint8_t nameLength[2];
    uint8_t typeLength[2];
    uint8_t commentLength[2];
  } symbol_info;
} AdsGetSymbolInfoByNameRepType;
#endif
