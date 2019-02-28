#ifndef INDEXER_H
#define INDEXER_H

int indexerHandleADS_ADR_getUInt(unsigned adsport,
                                 unsigned indexOffset,
                                 unsigned len_in_PLC,
                                 unsigned *uValue);
int indexerHandleADS_ADR_getFloat(unsigned adsport,
                                  unsigned indexOffset,
                                  unsigned len_in_PLC,
                                  double *fValue);
#endif
