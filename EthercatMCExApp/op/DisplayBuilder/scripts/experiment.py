# -----------------------------------------------------------------------------
# Jython - CSStudio
# -----------------------------------------------------------------------------
# simple experiment to scan as a snake movement
# -----------------------------------------------------------------------------
# ESS ERIC - ICS HWI group
# -----------------------------------------------------------------------------
# WP12 - douglas.bezerra.beniz@esss.se
# -----------------------------------------------------------------------------
from org.csstudio.display.builder.runtime.script import PVUtil, ScriptUtil

import sys, time
from time import sleep

# -----------------------------------------------------------------------------
# class objects
# -----------------------------------------------------------------------------
logger = ScriptUtil.getLogger()

# -----------------------------------------------------------------------------
# auxiliar procedure to check if user clicked at stop button
# -----------------------------------------------------------------------------
def verifyStop():
    stopExperiment = PVUtil.getInt(pvs[18])
    if stopExperiment:
        pvs[18].setValue(0)
        pvs[19].setValue("Canceled by user!")
        return 1
    else:
        return 0

# -----------------------------------------------------------------------------
# procedures
# -----------------------------------------------------------------------------
def experimentProcedure():
    try:
        # -------------------------------------------------------------------------
        # list of input PVs (keep it updated!)
        # -------------------------------------------------------------------------
        # pvs[0] = loc://run
        # pvs[1] = loc://startPosMotor1
        # pvs[2] = loc://endPosMotor1
        # pvs[3] = loc://numPointsMotor1
        # pvs[4] = loc://startPosMotor2
        # pvs[5] = loc://endPosMotor2
        # pvs[6] = loc://numPointsMotor2
        # pvs[7] = $(P)$(M1).RBV
        # pvs[8] = $(P)$(M1).VAL
        # pvs[9] = $(P)$(M1).DMOV
        # pvs[10] = $(P)$(M1).STOP
        # pvs[11] = $(P)$(M2).RBV
        # pvs[12] = $(P)$(M2).VAL
        # pvs[13] = $(P)$(M2).DMOV
        # pvs[14] = $(P)$(M2).STOP
        # pvs[15] = loc://integrationTime
        # pvs[16] = loc://arrayPointsX
        # pvs[17] = loc://arrayPointsY
        # pvs[18] = loc://stop
        # pvs[19] = loc://updateMessage
        # pvs[20] = loc://xAxisMinimum
        # pvs[21] = loc://xAxisMaximum
        # pvs[22] = loc://yAxisMinimum
        # pvs[23] = loc://yAxisMaximum
        # -------------------------------------------------------------------------
        # logical representation of PVs
        # -------------------------------------------------------------------------
        trigger             = PVUtil.getInt(pvs[0])
        startPosMotor1      = int(round(PVUtil.getDouble(pvs[1])))          # motor 1 is the x axis
        endPosMotor1        = int(round(PVUtil.getDouble(pvs[2])))
        numPointsMotor1     = int(round(PVUtil.getInt(pvs[3])))
        startPosMotor2      = int(round(PVUtil.getDouble(pvs[4])))          # motor 2 is the y axis
        endPosMotor2        = int(round(PVUtil.getDouble(pvs[5])))
        numPointsMotor2     = int(round(PVUtil.getInt(pvs[6])))
        rbvMotor1           = PVUtil.getDouble(pvs[7])
        valMotor1           = PVUtil.getDouble(pvs[8])
        dmovMotor1          = PVUtil.getInt(pvs[9])
        stopMotor1          = PVUtil.getInt(pvs[10])
        rbvMotor2           = PVUtil.getDouble(pvs[11])
        valMotor2           = PVUtil.getDouble(pvs[12])
        dmovMotor2          = PVUtil.getInt(pvs[13])
        stopMotor2          = PVUtil.getInt(pvs[14])
        integrationTime     = PVUtil.getInt(pvs[15])
        stopExperiment      = PVUtil.getInt(pvs[18])
        updateMessage       = PVUtil.getInt(pvs[19])
        xAxisMinimum        = PVUtil.getInt(pvs[20])
        xAxisMaximum        = PVUtil.getInt(pvs[21])
        yAxisMinimum        = PVUtil.getInt(pvs[22])
        yAxisMaximum        = PVUtil.getInt(pvs[23])

        if trigger:
            pvs[0].setValue(0)
            pvs[18].setValue(0)
            pvs[19].setValue("")
            # test consistency:
            if (endPosMotor1 > startPosMotor1) and (endPosMotor2 > startPosMotor2):
                pvs[19].setValue("Moving axis to initial position...")
                logger.info("start motor1: %d" % startPosMotor1)
                logger.info("end motor1: %d" % endPosMotor1)
                logger.info("start motor2: %d" % startPosMotor2)
                logger.info("end motor1: %d" % endPosMotor2)
                pvs[8].setValue(startPosMotor1)
                pvs[12].setValue(startPosMotor2)
                dmovMotor1 = PVUtil.getInt(pvs[9])
                dmovMotor2 = PVUtil.getInt(pvs[13])
                while not dmovMotor1 and not dmovMotor2:
                    sleep(0.1)
                    dmovMotor1 = PVUtil.getInt(pvs[9])
                    dmovMotor2 = PVUtil.getInt(pvs[13])
                    if verifyStop():
                        return 0
                # generate points for axis x
                goingPoints = range(startPosMotor1, endPosMotor1, (endPosMotor1-startPosMotor1)/numPointsMotor1)
                logger.info("going points: %s" % str(goingPoints))
                returningPoints = range(endPosMotor1, startPosMotor1, (endPosMotor1-startPosMotor1)/numPointsMotor1 *-1)
                logger.info("returning points: %s" % str(returningPoints))
                curPoints       = []
                arrayPointsX    = []
                arrayPointsY    = []
                going           = True
                pvs[19].setValue("Starting the scan procedure...")
                pvs[20].setValue(startPosMotor1)      # xAxisMinimum
                pvs[21].setValue(endPosMotor1)        # xAxisMaximum
                pvs[22].setValue(startPosMotor2)      # yAxisMinimum
                # ---------------------------------------------------------------------
                # external loop is the axis y
                for pointY in range(startPosMotor2, endPosMotor2, (endPosMotor2-startPosMotor2)/numPointsMotor2):
                    if verifyStop():
                        return 0
                    pvs[23].setValue(pointY)        # yAxisMaximum
                    pvs[12].setValue(pointY)        # $(P)$(M2).VAL
                    dmovMotor2 = PVUtil.getInt(pvs[13])
                    while not dmovMotor2:
                        sleep(0.1)
                        dmovMotor2 = PVUtil.getInt(pvs[13])
                        if verifyStop():
                            return 0
                    # internal loop is the axis x
                    if going:
                        going = False
                        curPoints = goingPoints
                    else:
                        going = True
                        curPoints = returningPoints
                    # -----------------------------------------------------------------
                    for pointX in curPoints:
                        pvs[8].setValue(pointX)
                        dmovMotor1 = PVUtil.getInt(pvs[9])
                        while not dmovMotor1:
                            sleep(0.1)
                            dmovMotor1 = PVUtil.getInt(pvs[9])
                            if verifyStop():
                                return 0
                        # include points X and Y at the arrays to plot the graphic
                        arrayPointsX.append(pointX)
                        arrayPointsY.append(pointY)
                        # update PVs
                        pvs[16].setValue(arrayPointsX)
                        pvs[17].setValue(arrayPointsY)
                        # integration time
                        sleep(integrationTime)
                        if verifyStop():
                            return 0
                    pvs[19].setValue("")
                pvs[19].setValue("Scan procedure has been concluded!")
            else:
                pvs[19].setValue("Input parameters are inconsistent. Please, fix them and try again...")
                pass
    except Exception as e:
        pvs[19].setValue("Error! %s " % str(e))
        logger.warning("Error! %s " % str(e))

# -----------------------------------------------------------------------------
# calling the main procedure
# -----------------------------------------------------------------------------
sleep(0.2)              # this was necessary because more than one procedure were being started, probably due to the period of scan of CSStudio thread
experimentProcedure()