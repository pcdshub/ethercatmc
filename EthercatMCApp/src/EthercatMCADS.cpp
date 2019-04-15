#include "EthercatMCController.h"
#include "EthercatMCIndexerAxis.h"
#include <asynOctetSyncIO.h>


asynStatus EthercatMCController::writeControllerBinary(const char *buffer,
                                                       size_t bufferLen)
{
  char old_OutputEos[10];
  int old_OutputEosLen = 0;
  size_t nwrite = 0;
  asynStatus status;

  status = pasynOctetSyncIO->getOutputEos(pasynUserController_,
                                          &old_OutputEos[0],
                                          (int)sizeof(old_OutputEos),
                                          &old_OutputEosLen);
  if (status) {
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n",
              modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_OutputEos;
  }
  status = pasynOctetSyncIO->setOutputEos(pasynUserController_,
                                          "", 0);

  if (status) {
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n",
              modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_OutputEos;
  }
  status = pasynOctetSyncIO->write(pasynUserController_,
                                   buffer, bufferLen,
                                   DEFAULT_CONTROLLER_TIMEOUT,
                                   &nwrite);
  if (status) {
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%sstatus=%s (%d)\n",
              modNamEMC,
              pasynManager->strStatus(status), (int)status);
    goto restore_OutputEos;
  }

restore_OutputEos:
  {
    asynStatus cmdStatus;
    cmdStatus = pasynOctetSyncIO->setOutputEos(pasynUserController_,
                                               old_OutputEos,
                                               old_OutputEosLen);
    asynPrint(pasynUserController_, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%scmdStatus=%s (%d)\n",
              modNamEMC,
              pasynManager->strStatus(cmdStatus), (int)cmdStatus);
  }

  return status;
}
