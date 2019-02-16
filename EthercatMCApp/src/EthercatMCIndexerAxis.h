extern "C" {
  int EthercatMCCreateIndexerAxis(const char *EthercatMCName, int axisNo,
                                  int axisFlags, const char *axisOptionsStr);
};

class epicsShareClass EthercatMCIndexerAxis : public asynMotorAxis
{
public:
  /* These are the methods we override from the base class */
  EthercatMCIndexerAxis(class EthercatMCController *pC, int axisNo,
                        int axisFlags, const char *axisOptionsStr);
  void report(FILE *fp, int level);
  asynStatus move(double position, int relative, double min_velocity, double max_velocity, double acceleration);
  asynStatus moveVelocity(double min_velocity, double max_velocity, double acceleration);
  asynStatus setPosition(double);

  asynStatus home(double min_velocity, double max_velocity, double acceleration, int forwards);
  asynStatus stopAxisInternal(const char *function_name, double acceleration);
  asynStatus stop(double acceleration);
  void handleDisconnect(asynStatus status);
  void setIndexerTypeCodeOffset(unsigned iTypCode, unsigned iOffset);
  asynStatus poll(bool *moving);
  asynStatus resetAxis(void);
  asynStatus setClosedLoop(bool closedLoop);
  asynStatus setIntegerParam(int function, int value);
private:
  EthercatMCController *pC_;

    struct {
      double     scaleFactor;
      const char *externalEncoderStr;
      struct {
        unsigned int oldStatusDisconnected : 1;
        unsigned int initialPollNeeded :1;
    }  dirty;
      unsigned int adsPort;
      unsigned iTypCode;
      unsigned iOffset;
      unsigned old_tatusReasonAux;
      unsigned old_cmdSubParamIndex;
      unsigned int hasError :1;
  } drvlocal;

  friend class EthercatMCController;
};

