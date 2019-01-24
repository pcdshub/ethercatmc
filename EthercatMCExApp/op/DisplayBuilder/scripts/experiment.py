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
#from pvaccess import BOOLEAN, BYTE, UBYTE, SHORT, USHORT, INT, UINT, LONG, ULONG, FLOAT, DOUBLE, STRING, PvObject, PvaServer

import sys, time

from time import sleep
from array import array
from jarray import zeros


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
        # pvs[20] = $(P)$(M1):x_axis_minimum                       # LabS-ESSIIP:MC-MCU-019:m1:x_axis_minimum
        # pvs[21] = $(P)$(M1):x_axis_maximum                       # LabS-ESSIIP:MC-MCU-019:m1:x_axis_maximum
        # pvs[22] = $(P)$(M2):y_axis_minimum                       # LabS-ESSIIP:MC-MCU-019:m2:y_axis_minimum
        # pvs[23] = $(P)$(M2):y_axis_maximum                       # LabS-ESSIIP:MC-MCU-019:m2:y_axis_maximum
        # pvs[24] = $(P)arrayPointsX                               # LabS-ESSIIP:MC-MCU-019:arrayPointsX
        # pvs[25] = $(P)arrayPointsY                               # LabS-ESSIIP:MC-MCU-019:arrayPointsY
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
        # -------------------------------------------------------------------------
        # getting the current display
        # -------------------------------------------------------------------------
        display = widget.getDisplayModel()

        if trigger:
            pvs[0].setValue(0)          # trigger
            pvs[18].setValue(0)         # stop
            pvs[19].setValue("")        # message
            # test consistency:
            if (endPosMotor1 > startPosMotor1) and (endPosMotor2 > startPosMotor2):
                pvs[19].setValue("Moving axis to initial position...")
                logger.info("start motor1: %d" % startPosMotor1)
                logger.info("end motor1: %d" % endPosMotor1)
                logger.info("start motor2: %d" % startPosMotor2)
                logger.info("end motor1: %d" % endPosMotor2)
                sleep(0.1)          # just to be sure it started
                pvs[8].setValue(startPosMotor1)         # $(P)$(M1).VAL
                pvs[12].setValue(startPosMotor2)        # $(P)$(M2).VAL
                dmovMotor1 = PVUtil.getInt(pvs[9])
                dmovMotor2 = PVUtil.getInt(pvs[13])
                while not dmovMotor1 and not dmovMotor2:
                    sleep(0.1)
                    dmovMotor1 = PVUtil.getInt(pvs[9])
                    dmovMotor2 = PVUtil.getInt(pvs[13])
                    if verifyStop():
                        return 0
                # generate points for axis x
                goingPoints = range(startPosMotor1, endPosMotor1+1, (endPosMotor1-startPosMotor1)/numPointsMotor1)
                logger.info("going points: %s" % str(goingPoints))
                returningPoints = range(endPosMotor1, startPosMotor1-1, (endPosMotor1-startPosMotor1)/numPointsMotor1 *-1)
                logger.info("returning points: %s" % str(returningPoints))
                curPoints       = []
                arrayPointsX    = []
                arrayPointsY    = []
                going           = True
                pvs[19].setValue("Starting the scan procedure...")
                # -----------------------------------------------------------------
                # gets the XY graphic component from display (which has been captured using correspondent widget model)
                scanXYPlot = display.runtimeChildren().getChildByName('scanXYPlot')
                scanXYPlot.setPropertyValue('x_axis.minimum', startPosMotor1-1)
                scanXYPlot.setPropertyValue('x_axis.maximum', endPosMotor1+1)
                scanXYPlot.setPropertyValue('y_axes[0].minimum', startPosMotor2-1)
                # ---------------------------------------------------------------------
                pvs[20].setValue(startPosMotor1-1)              # $(P)$(M1):x_axis_minimum
                pvs[21].setValue(endPosMotor1+1)                # $(P)$(M1):x_axis_maximum
                pvs[22].setValue(startPosMotor2-1)              # $(P)$(M2):y_axis_minimum
                # ---------------------------------------------------------------------
                # clean the array...
                pvs[24].setValue(zeros(124*124,'h'))          # $(P)arrayPointsX (LabS-ESSIIP:MC-MCU-019:arrayPointsX)
                pvs[25].setValue(zeros(124*124,'h'))          # $(P)arrayPointsY (LabS-ESSIIP:MC-MCU-019:arrayPointsY)
                # ---------------------------------------------------------------------
                # external loop is the axis y
                for pointY in range(startPosMotor2, endPosMotor2, (endPosMotor2-startPosMotor2)/numPointsMotor2):
                    if verifyStop():
                        return 0
                    pvs[12].setValue(pointY)        # $(P)$(M2).VAL
                    # -----------------------------------------------------------------
                    scanXYPlot.setPropertyValue('y_axes[0].maximum', pointY+1)
                    # -----------------------------------------------------------------
                    pvs[23].setValue(pointY+1)              # $(P)$(M2):y_axis_maximum
                    # -----------------------------------------------------------------
                    sleep(0.1)          # just to be sure it started
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
                        pvs[8].setValue(pointX)             # $(P)$(M1).VAL
                        sleep(0.1)          # just to be sure it started
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
                        pvs[16].setValue(arrayPointsX)          # loc://arrayPointsX
                        pvs[17].setValue(arrayPointsY)          # loc://arrayPointsY
                        # ------------------------
                        pvs[24].setValue(array('h', arrayPointsX))          # $(P)arrayPointsX (LabS-ESSIIP:MC-MCU-019:arrayPointsX)
                        pvs[25].setValue(array('h', arrayPointsY))          # $(P)arrayPointsY (LabS-ESSIIP:MC-MCU-019:arrayPointsY)
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