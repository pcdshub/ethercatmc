# @field AXISCONFIG
# @type  STRING
# File name for axis configuration, leave empty

# @field PREFIX
# @type  STRING

# @field MOTOR_NAME
# @type  STRING
# m1, m2, m3, X, Y, Z

# @field MOTOR_PORT
# @type  STRING
# MCU1, MCU2

# @field AXIS_NO
# @type  STRING
# 1,2,3,4

# @field DESC
# @type  STRING
# Description shown in UI

# @field PREC
# @type  INTEGER
# 3

# @field ECAXISFIELDINIT
# @type  STRING
# Field to be initialized like ,RTRY=5,DLY=0.2


EthercatMCCreateGvlAxis("$(MOTOR_PORT)", "$(AXIS_NO)", "6", "$(AXISCONFIG)")

dbLoadRecords("EthercatMC.template", "PREFIX=$(PREFIX), MOTOR_NAME=$(MOTOR_NAME), R=$(MOTOR_NAME)-, MOTOR_PORT=$(MOTOR_PORT), ASYN_PORT=$(ASYN_PORT), AXIS_NO=$(AXIS_NO), DESC=$(DESC), PREC=$(PREC) $(ECAXISFIELDINIT)")

dbLoadRecords("EthercatMCreadback.template", "PREFIX=$(PREFIX), MOTOR_NAME=$(MOTOR_NAME), R=$(MOTOR_NAME)-, MOTOR_PORT=$(MOTOR_PORT), ASYN_PORT=$(ASYN_PORT), AXIS_NO=$(AXIS_NO), DESC=$(DESC), PREC=$(PREC) ")
