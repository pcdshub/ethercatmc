# -----------------------------------------------------------------------------
# Douglas version
# -----------------------------------------------------------------------------
require asyn,4.33
require calc,3.7.1
require EthercatMC,2.1.0

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
epicsEnvSet("VELO",          "$(SM_VELO=24.9)")
epicsEnvSet("JVEL",          "$(SM_JVEL=10)")
epicsEnvSet("JAR",           "$(SM_JAR=10.2)")
epicsEnvSet("ACCL",          "$(SM_ACCL=1)")
epicsEnvSet("MRES",          "$(SM_MRES=0.001)")
epicsEnvSet("ERES",          "$(SM_ERES=0.0046875)")
epicsEnvSet("RDBD",          "$(SM_RDBD=0.1)")
epicsEnvSet("NTMF",          "$(SM_NTMF=1)")
epicsEnvSet("DLLM",          "$(SM_DLLM=15)")
epicsEnvSet("DHLM",          "$(SM_DHLM=165)")
epicsEnvSet("HOMEPROC",      "$(SM_HOMEPROC=1)")
epicsEnvSet("HOMEPOS",       "$(SM_HOMEPOS=0)")
epicsEnvSet("HVELTO",        "$(SM_HVELTO=5)")
epicsEnvSet("HVELFRM",       "$(SM_HVELFRM=2)")
epicsEnvSet("HOMEACC",       "$(SM_HOMEACC=20)")
epicsEnvSet("HOMEDEC",       "$(SM_HOMEDEC=50)")
epicsEnvSet("AXISCONFIG",    "$(SM_AXISCONFIG="")")


< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCAxis.cmd
#< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCAxisdebug.cmd
#< /epics/iocs/cmds/labs-utg-mc-mcu-01/m-epics-ethercatmc/startup/EthercatMCAxishome.cmd
