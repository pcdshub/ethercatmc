# -----------------------------------------------------------------------------
# Douglas version
# -----------------------------------------------------------------------------
require asyn,4.33
require calc,3.7.1
require EthercatMC,motor-28d6f42c6db-ethercatmc-1dcc075a

# -----------------------------------------------------------------------------
# IOC common settings
# -----------------------------------------------------------------------------
epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")
epicsEnvSet("IPADDR",        "$(SM_IPADDR=172.30.242.17)")
epicsEnvSet("IPPORT",        "$(SM_IPPORT=200)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")

# -----------------------------------------------------------------------------
# EtherCAT MC Controller
# -----------------------------------------------------------------------------
< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCController.cmd


# -----------------------------------------------------------------------------
# Axis 1 configuration and instantiation
# -----------------------------------------------------------------------------
epicsEnvSet("MOTOR_PORT",     "$(SM_MOTOR_PORT=MCU1)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=1)")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=LabS-ESSIIP:)")
epicsEnvSet("P",             "$(SM_PREFIX=LabS-ESSIIP:)")
epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=MC-MCU-01:m1)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
epicsEnvSet("R",             "$(SM_R=m1-)")
epicsEnvSet("DESC",          "$(SM_DESC=m1)")
epicsEnvSet("PREC",          "$(SM_PREC=3)")

#Information that the controller may provide, but doesn't
epicsEnvSet("DLLM",          "$(SM_DLLM=0)")
epicsEnvSet("DHLM",          "$(SM_DHLM=165)")

# Initialize the soft limits
epicsEnvSet("ECAXISFIELDINIT", ",DHLM=$(DHLM),DLLM=$(DLLM)")

epicsEnvSet("AXISCONFIG",    "adsport=852;HomProc=1;HomPos=0.0;encoder=ADSPORT=501/.ADR.16#3040040,16#8000001C,2,2")


< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCAxis.cmd
#< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCAxisdebug.cmd
#< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCAxishome.cmd
