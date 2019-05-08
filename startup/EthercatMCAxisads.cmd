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

dbLoadRecords("EthercatMCads.template", "PREFIX=$(PREFIX), MOTOR_NAME=$(MOTOR_NAME), R=$(MOTOR_NAME)-, MOTOR_PORT=$(MOTOR_PORT), ASYN_PORT=$(ASYN_PORT), AXIS_NO=$(AXIS_NO), DESC=$(DESC), PREC=$(PREC) ")
