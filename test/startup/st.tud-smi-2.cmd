require asyn,4.33
require EthercatMC,2.1.0

epicsEnvSet("ECM_NUMAXES",   "3")
epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")

epicsEnvSet("IPADDR",        "$(SM_IPADDR=127.0.0.1)")
epicsEnvSet("IPPORT",        "$(SM_IPPORT=5000)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=TUD-SMI:MC-MCU-02:)")
epicsEnvSet("EGU",           "$(SM_EGU=mm)")
epicsEnvSet("PREC",          "$(SM_PREC=3)")
< EthercatMCController.cmd

epicsEnvSet("AXISCONFIG",    "HomProc=1;HomPos=0")
epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=m1)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=1)")
epicsEnvSet("DESC",          "$(SM_DESC=motor1)")
< EthercatMCAxis.cmd
< EthercatMCAxisdebug.cmd

epicsEnvSet("AXISCONFIG",    "HomProc=15;HomPos=0")
epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=m2)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=2)")
epicsEnvSet("DESC",          "$(SM_DESC=motor2)")
< EthercatMCAxis.cmd
< EthercatMCAxisdebug.cmd

epicsEnvSet("AXISCONFIG",    "HomProc=15;HomPos=0")
epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=m3)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=3)")
epicsEnvSet("DESC",          "$(SM_DESC=motor3)")
< EthercatMCAxis.cmd
< EthercatMCAxisdebug.cmd


