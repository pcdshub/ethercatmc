require EthercatMC,USER

epicsEnvSet("ECM_NUMAXES",   "1")
epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")

epicsEnvSet("IPADDR",        "$(SM_IPADDR=172.30.242.17)")
epicsEnvSet("IPPORT",        "$(SM_IPPORT=200)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=LabS-ESSIIP:MC-MCU-01:)")
epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=m1)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=1)")
epicsEnvSet("DESC",          "$(SM_DESC=Lower=Right)")
epicsEnvSet("EGU",           "$(SM_EGU=mm)")
epicsEnvSet("PREC",          "$(SM_PREC=3)")

# And this reads the input of an incremental encoder terminal
# on the EtherCAT bus. Works with the simulator.
# For real terminals the adresses must be adapted
epicsEnvSet("AXISCONFIG",    "adsPort=851;stepSize=1.0;HomProc=1;cfgFile=./ESSIIP-CfgDbg.cfg;encoder=ADSPORT=501/.ADR.16#3040040,16#8000001C,2,2")


< EthercatMCController.cmd
< EthercatMCAxis.cmd
< EthercatMCAxisdebug.cmd
