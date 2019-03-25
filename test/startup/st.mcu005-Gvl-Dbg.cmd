require EthercatMC,USER

epicsEnvSet("ECM_NUMAXES",   "1")
epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")

epicsEnvSet("IPADDR",        "$(SM_IPADDR=127.0.0.1)")
epicsEnvSet("IPPORT",        "$(SM_IPPORT=5000)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=IOC:)")
epicsEnvSet("EGU",           "$(SM_EGU=mm)")
epicsEnvSet("PREC",          "$(SM_PREC=3)")
epicsEnvSet("SM_NOAXES",     "1")
epicsEnvSet("ADSPORT",       "$(ECM_ADSPORT=851)")

< EthercatMCController.cmd

epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=m1)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=1)")
epicsEnvSet("DESC",          "$(SM_DESC=Lower=Right)")
epicsEnvSet("AXISCONFIG",    "sFeatures=Gvl;stv1;adsPort=$(ADSPORT);HomProc=1")
< EthercatMCGvlAxis.cmd
< EthercatMCAxisdebug.cmd
