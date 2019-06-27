#!/usr/bin/python

"""
Author: Matt Pearson
Date: Feb 2015

Description: Class to hold utility functions
"""

import sys
import math
import time
import os
import filecmp

from epics import caput, caget
import epics

from motor_globals import motor_globals


motorRMOD_D = 0 # "Default"
motorRMOD_A = 1 # "Arithmetic"
motorRMOD_G = 2 # "Geometric"
motorRMOD_I = 3 # "In-Position"


polltime = 0.2

class motor_lib(object):
    """
    Library of useful test functions for motor record applications
    """

    __g = motor_globals()
    MSTA_BIT_HOMED        =  1 << (15 -1)    #4000
    MSTA_BIT_MINUS_LS     =  1 << (14 -1)    #2000
    MSTA_BIT_COMM_ERR     =  1 << (13 -1)    #1000
    MSTA_BIT_GAIN_SUPPORT =  1 << (12 -1)    #0800
    MSTA_BIT_MOVING       =  1 << (11 -1)    #0400
    MSTA_BIT_PROBLEM      =  1 << (10 -1)    #0200
    MSTA_BIT_PRESENT      =  1 << (9 -1)     #0100
    MSTA_BIT_HOME         =  1 << (8 -1)     #0080
    MSTA_BIT_SLIP_STALL   =  1 << (7 -1)     #0040
    MSTA_BIT_AMPON        =  1 << (6 -1)     #0020
    MSTA_BIT_UNUSED       =  1 << (5 -1)     #0010
    MSTA_BIT_HOMELS       =  1 << (4 -1)     #0008
    MSTA_BIT_PLUS_LS      =  1 << (3 -1)     #0004
    MSTA_BIT_DONE         =  1 << (2 -1)     #0002
    MSTA_BIT_DIRECTION    =  1 << (1 -1)     #0001

    MIP_BIT_JOGF       = 0x0001
    MIP_BIT_JOGR       = 0x0002
    MIP_BIT_JOG_BL1    = 0x0004
    MIP_BIT_HOMF       = 0x0008
    MIP_BIT_HOMR       = 0x0010
    MIP_BIT_MOVE       = 0x0020
    MIP_BIT_RETRY      = 0x0040
    MIP_BIT_LOAD_P     = 0x0080
    MIP_BIT_MOVE_BL    = 0x0100
    MIP_BIT_STOP       = 0x0200
    MIP_BIT_DELAY_REQ  = 0x0400
    MIP_BIT_DELAY_ACK  = 0x0800
    MIP_BIT_JOG_REQ    = 0x1000
    MIP_BIT_JOG_STOP   = 0x2000
    MIP_BIT_JOG_BL2    = 0x4000
    MIP_BIT_EXTERNAL   = 0x8000

    # Values to be used for backlash test
    # Note: Make sure to use different values to hae a good
    # test coverage

    myVELO = 10.0   # positioning velocity
    myACCL =  1.0   # Time to VELO, seconds
    myAR   = myVELO / myACCL # acceleration, mm/sec^2

    myJVEL = 5.0    # Jogging velocity
    myJAR  = 6.0    # Jogging acceleration, mm/sec^2

    myBVEL = 2.0    # backlash velocity
    myBACC = 1.5    # backlash acceleration, seconds
    myBAR  = myBVEL / myBACC  # backlash acceleration, mm/sec^2
    myRTRY   = 3
    myDLY    =  0.0
    myBDST   = 24.0 # backlash destination, mm
    myFRAC   = 1.0  #
    myPOSlow = 48   #
    myPOSmid = 72   # low + BDST
    myPOShig = 96   # low + 2*BDST

    def initializeMotorRecordOneField(self, motor, tc_no, field, value):
        channelname = motor + field
        oldVal = epics.caget(channelname)
        if (oldVal != None):
            print '%s: initializeMotorRecordOneField field=%s oldVal=%f value=%f' % (
                tc_no, channelname, oldVal, value)
            if (oldVal != value):
                epics.caput(channelname, value)

        else:
            print '%s: initializeMotorRecordOneField field=%s not found value=%f' % (
                tc_no, channelname, value)

    def initializeMotorRecordSimulatorAxis(self, motor, tc_no):
        # If there are usful values in the controller, use them
        dhlm = epics.caget(motor + '-CfgDHLM')
        dllm = epics.caget(motor + '-CfgDLLM')
        if (dhlm == None or dllm == None or dhlm <= dllm):
            dhlm = 53.0
            dllm = -54.0


        self.initializeMotorRecordOneField(motor, tc_no, '.VMAX', 50.0)
        self.initializeMotorRecordOneField(motor, tc_no, '.VELO', 20.0)
        self.initializeMotorRecordOneField(motor, tc_no, '.ACCL', 5.0)
        self.initializeMotorRecordOneField(motor, tc_no, '.JVEL', 5.0)
        self.initializeMotorRecordOneField(motor, tc_no, '.JAR',  20.0)

        self.initializeMotorRecordOneField(motor, tc_no, '.RDBD', 0.1)
        #self.initializeMotorRecordOneField(motor, tc_no, '.SPDB', 0.1)
        self.initializeMotorRecordOneField(motor, tc_no, '.BDST', 0.0)

        self.setSoftLimitsOff(motor)
        self.initializeMotorRecordOneField(motor, tc_no, '-CfgDHLM', dhlm)
        self.initializeMotorRecordOneField(motor, tc_no, '-CfgDLLM', dllm)
        self.initializeMotorRecordOneField(motor, tc_no, '-CfgDHLM-En', 1)
        self.initializeMotorRecordOneField(motor, tc_no, '-CfgDLLM-En', 1)
        self.initializeMotorRecordOneField(motor, tc_no, '.DHLM', dhlm)
        self.initializeMotorRecordOneField(motor, tc_no, '.DLLM', dllm)

    def getMSTAtext(self, msta):
        ret = ''
        if (msta & self.MSTA_BIT_HOMED):
            ret = ret + 'Hmd'
        else:
            ret = ret +'...'
        if (msta & self.MSTA_BIT_MINUS_LS):
            ret = ret + 'Lls'
        else:
            ret = ret +'...'
        #if (msta & self.MSTA_BIT_GAIN_SUPPORT):
        #    ret = ret + 'G'
        #else:
        #    ret = ret +'.'
        if (msta & self.MSTA_BIT_MOVING):
            ret = ret + 'Mov'
        else:
            ret = ret +'...'
        if (msta & self.MSTA_BIT_PROBLEM):
            ret = ret + 'Prb'
        else:
            ret = ret +'.'
        if (msta & self.MSTA_BIT_PRESENT):
            ret = ret + 'Enc'
        else:
            ret = ret +'...'
        if (msta & self.MSTA_BIT_HOME):
            ret = ret + 'Hom'
        else:
            ret = ret +'..'
        if (msta & self.MSTA_BIT_SLIP_STALL    ):
            ret = ret + 'Slp'
        else:
            ret = ret +'....'
        if (msta & self.MSTA_BIT_AMPON):
            ret = ret + 'Amp'
        else:
            ret = ret +'...'
        if (msta & self.MSTA_BIT_HOMELS):
            ret = ret + 'Hsw'
        else:
            ret = ret +'...'
        if (msta & self.MSTA_BIT_PLUS_LS):
            ret = ret + 'Hls'
        else:
            ret = ret +'...'
        if (msta & self.MSTA_BIT_DONE):
            ret = ret + 'Don'
        else:
            ret = ret +'...'
        return ret

    def getMIPtext(self, mip):
        ret = ''
        if (mip & self.MIP_BIT_JOGF):
            ret = ret + 'JOGF '
        if (mip & self.MIP_BIT_JOGR):
            ret = ret + 'JOGR '
        if (mip & self.MIP_BIT_JOG_BL1):
            ret = ret + 'JOG_BL1 '
        if (mip & self.MIP_BIT_HOMF):
            ret = ret + 'HOMF '
        if (mip & self.MIP_BIT_HOMR):
            ret = ret + 'HOMR '
        if (mip & self.MIP_BIT_MOVE):
            ret = ret + 'MOVE '
        if (mip & self.MIP_BIT_LOAD_P):
            ret = ret + 'LOAD_P '
        if (mip & self.MIP_BIT_MOVE_BL):
            ret = ret + 'MOVE_BL '
        if (mip & self.MIP_BIT_DELAY_REQ):
            ret = ret + 'DELAY_REQ '
        if (mip & self.MIP_BIT_DELAY_ACK):
            ret = ret + 'DELAY_ACK '
        if (mip & self.MIP_BIT_JOG_REQ):
            ret = ret + 'JOG_REQ '
        if (mip & self.MIP_BIT_JOG_STOP):
            ret = ret + 'JOG_STOP '
        if (mip & self.MIP_BIT_JOG_BL2):
            ret = ret + 'JOG_BL2 '
        if (mip & self.MIP_BIT_EXTERNAL):
            ret = ret + 'EXTERNAL '
        return ret

    def calcAlmostEqual(self, motor, tc_no, expected, actual, maxdelta):
        delta = math.fabs(expected - actual)
        inrange = delta < maxdelta
        print '%s: assertAlmostEqual expected=%f actual=%f delta=%f maxdelta=%f inrange=%d' % (
            tc_no, expected, actual, delta, maxdelta, inrange)
        return inrange

    def waitForStart(self, motor, tc_no, wait_for_start):
        while wait_for_start > 0:
            wait_for_start -= polltime
            dmov = int(epics.caget(motor + '.DMOV', use_monitor=False))
            movn = int(epics.caget(motor + '.MOVN', use_monitor=False))
            rbv = epics.caget(motor + '.RBV')
            print '%s: wait_for_start=%f dmov=%d movn=%d rbv=%f' % (
                tc_no, wait_for_start, dmov, movn, rbv)
            if movn and not dmov:
                return True
            time.sleep(polltime)
            wait_for_start -= polltime
        return False

    def waitForStop(self, motor, tc_no, wait_for_stop):
        while wait_for_stop > 0:
            wait_for_stop -= polltime
            dmov = int(epics.caget(motor + '.DMOV', use_monitor=False))
            movn = int(epics.caget(motor + '.MOVN', use_monitor=False))
            rbv = epics.caget(motor + '.RBV', use_monitor=False)
            print '%s: wait_for_stop=%f dmov=%d movn=%d rbv=%f' % (
                tc_no, wait_for_stop, dmov, movn, rbv)
            if not movn and dmov:
                return True
            time.sleep(polltime)
            wait_for_stop -= polltime
        return False

    def waitForStartAndDone(self, motor, tc_no, wait_for_done):
        wait_for_start = 2
        while wait_for_start > 0:
            wait_for_start -= polltime
            dmov = int(caget(motor + '.DMOV'))
            movn = int(caget(motor + '.MOVN'))
            print '%s: wait_for_start=%f dmov=%d movn=%d dpos=%f' % (
                tc_no, wait_for_start, dmov, movn, caget(motor + '.DRBV', use_monitor=False))
            if movn and not dmov:
               break
            time.sleep(polltime)

        wait_for_done = math.fabs(wait_for_done) #negative becomes positive
        wait_for_done += 1 # One extra second for rounding
        while wait_for_done > 0:
            dmov = int(caget(motor + '.DMOV'))
            movn = int(caget(motor + '.MOVN'))
            print '%s: wait_for_done=%f dmov=%d movn=%d dpos=%f' % (
                tc_no, wait_for_done, dmov, movn, caget(motor + '.DRBV', use_monitor=False))
            if dmov and not movn:
                return True
            time.sleep(polltime)
            wait_for_done -= polltime
        return False

    def waitForMipZero(self, motor, tc_no, wait_for_MipZero):
        while wait_for_MipZero > -10.0: # Extra long wait
            wait_for_MipZero -= polltime
            mip = int(epics.caget(motor + '.MIP', use_monitor=False))
            print '%s: wait_for_MipZero=%f mip=%s (%x)' % (
                tc_no, wait_for_MipZero, self.getMIPtext(mip),mip)
            if not mip:
                return True
            time.sleep(polltime)
            wait_for_MipZero -= polltime
        return False

    def testComplete(self, fail):
        """
        Function to be called at end of test
        fail = true or false
        """
        if not fail:
            print "Test Complete"
            return self.__g.SUCCESS
        else:
            print "Test Failed"
            return self.__g.FAIL


    def jogDirection(self, motor, tc_no, direction, time_to_wait):
        if direction > 0:
            epics.caput(motor + '.JOGF', 1)
        else:
            epics.caput(motor + '.JOGR', 1)

        done = self.waitForStartAndDone(motor, tc_no + " jogDirection", 30 + time_to_wait + 3.0)

        if direction > 0:
            epics.caput(motor + '.JOGF', 0)
        else:
            epics.caput(motor + '.JOGR', 0)

    def move(self, motor, position, timeout):
        """
        Move motor to position. We use put_callback
        and check final position is within RDBD.
        """

        try:
            caput(motor, position, wait=True, timeout=timeout)
        except:
            e = sys.exc_info()
            print str(e)
            print "ERROR: caput failed."
            print (motor + " pos:" + str(position) + " timeout:" + str(timeout))
            return self.__g.FAIL

        rdbd = motor + ".RDBD"
        rbv = motor + ".RBV"

        final_pos = caget(rbv, use_monitor=False)
        deadband = caget(rdbd)

        success = True

        if ((final_pos < position-deadband) or (final_pos > position+deadband)):
            print "ERROR: final_pos out of deadband."
            success = False
        else:
            print "Final_pos inside deadband."
        print (motor + " pos=" + str(position) +
               " timeout=" + str(timeout) +
               " final_pos=" + str(final_pos) +
               " deadband=" + str(deadband))

        if (success):
            return self.postMoveCheck(motor)
        else:
            self.postMoveCheck(motor)
            return self.__g.FAIL


    def movePosition(self, motor, tc_no, destination, velocity, acceleration):
        time_to_wait = 30
        if velocity > 0:
            distance = math.fabs(caget(motor + '.RBV') - destination)
            time_to_wait += distance / velocity + 2 * acceleration
        caput(motor + '.VAL', destination)
        done = self.waitForStartAndDone(motor, tc_no + " movePosition", time_to_wait)

    def setPosition(self, motor, position, timeout):
        """
        Set position on motor and check it worked ok.
        """

        _set = motor + ".SET"
        _rbv = motor + ".RBV"
        _dval = motor + ".DVAL"
        _off = motor + ".OFF"

        offset = caget(_off)

        caput(_set, 1, wait=True, timeout=timeout)
        caput(_dval, position, wait=True, timeout=timeout)
        caput(_off, offset, wait=True, timeout=timeout)
        caput(_set, 0, wait=True, timeout=timeout)

        if (self.postMoveCheck(motor) != self.__g.SUCCESS):
            return self.__g.FAIL

        try:
            self.verifyPosition(motor, position+offset)
        except Exception as e:
            print str(e)
            return self.__g.FAIL

    def checkInitRecord(self, motor):
        """
        Check the record for correct state at start of test.
        """
        self.postMoveCheck()


    def verifyPosition(self, motor, position):
        """
        Verify that field == reference.
        """
        _rdbd = motor + ".RDBD"
        deadband = caget(_rdbd)
        _rbv = motor + ".RBV"
        current_pos = caget(_rbv)

        if ((current_pos < position-deadband) or (current_pos > position+deadband)):
            print "ERROR: verifyPosition out of deadband."
            msg = (motor + " pos=" + str(position) +
                   " currentPos=" + str(current_pos) +
                   " deadband=" + str(deadband))
            raise Exception(__name__ + msg)

        return self.__g.SUCCESS

    def verifyField(self, pv, field, reference):
        """
        Verify that field == reference.
        """
        full_pv = pv + "." + field
        if (caget(full_pv) != reference):
            msg = "ERROR: " + full_pv + " not equal to " + str(reference)
            raise Exception(__name__ + msg)

        return self.__g.SUCCESS


    def postMoveCheck(self, motor):
        """
        Check the motor for the correct state at the end of move.
        """

        DMOV = 1
        MOVN = 0
        STAT = 0
        SEVR = 0
        LVIO = 0
        MISS = 0
        RHLS = 0
        RLLS = 0

        try:
            self.verifyField(motor, "DMOV", DMOV)
            self.verifyField(motor, "MOVN", MOVN)
            self.verifyField(motor, "STAT", STAT)
            self.verifyField(motor, "SEVR", SEVR)
            self.verifyField(motor, "LVIO", LVIO)
            self.verifyField(motor, "MISS", MISS)
            self.verifyField(motor, "RHLS", RHLS)
            self.verifyField(motor, "RLLS", RLLS)
        except Exception as e:
            print str(e)
            return self.__g.FAIL

        return self.__g.SUCCESS


    def setValueOnSimulator(self, motor, tc_no, var, value):
        var = str(var)
        value = str(value)
        outStr = 'Sim.this.' + var + '=' + value
        print '%s: DbgStrToMCU motor=%s var=%s value=%s outStr=%s' % \
              (tc_no, motor, var, value, outStr)
        assert(len(outStr) < 40)
        epics.caput(motor + '-DbgStrToMCU', outStr, wait=True)
        err = int(epics.caget(motor + '-Err', use_monitor=False))
        print '%s: DbgStrToMCU motor=%s var=%s value=%s err=%d' % \
              (tc_no, motor, var, value, err)
        assert (not err)

    def motorInitAllForBDST(self, motor, tc_no):
        self.setValueOnSimulator(motor, tc_no, "nAmplifierPercent", 100)
        self.setValueOnSimulator(motor, tc_no, "bAxisHomed",          1)
        self.setValueOnSimulator(motor, tc_no, "fLowHardLimitPos",    -100)
        self.setValueOnSimulator(motor, tc_no, "fHighHardLimitPos",   100)
        self.setValueOnSimulator(motor, tc_no, "setMRES_23", 0)
        self.setValueOnSimulator(motor, tc_no, "setMRES_24", 0)

        epics.caput(motor + '-ErrRst', 1)
        # Prepare parameters for jogging and backlash
        self.setSoftLimitsOff(motor)
        epics.caput(motor + '.VELO', self.myVELO)
        epics.caput(motor + '.ACCL', self.myACCL)

        epics.caput(motor + '.JVEL', self.myJVEL)
        epics.caput(motor + '.JAR',  self.myJAR)

        epics.caput(motor + '.BVEL', self.myBVEL)
        epics.caput(motor + '.BACC', self.myBACC)
        epics.caput(motor + '.BDST', self.myBDST)
        epics.caput(motor + '.FRAC', self.myFRAC)
        epics.caput(motor + '.RTRY', self.myRTRY)
        epics.caput(motor + '.RMOD', motorRMOD_D)
        epics.caput(motor + '.DLY',  self.myDLY)

    def writeExpFileRMOD_X(self, motor, tc_no, rmod, dbgFile, expFile, maxcnt, frac, encRel, motorStartPos, motorEndPos):
        cnt = 0
        if motorEndPos - motorStartPos > 0:
            directionOfMove = 1
        else:
            directionOfMove = -1
        if self.myBDST > 0:
            directionOfBL = 1
        else:
            directionOfBL = -1

        print '%s: writeExpFileRMOD_X motor=%s encRel=%d motorStartPos=%f motorEndPos=%f directionOfMove=%d directionOfBL=%d' % \
              (tc_no, motor, encRel, motorStartPos, motorEndPos, directionOfMove, directionOfBL)

        if dbgFile != None:
            dbgFile.write('#%s: writeExpFileRMOD_X motor=%s rmod=%d encRel=%d motorStartPos=%f motorEndPos=%f directionOfMove=%d directionOfBL=%d\n' % \
              (tc_no, motor, rmod, encRel, motorStartPos, motorEndPos, directionOfMove, directionOfBL))

        if rmod == motorRMOD_I:
            maxcnt = 1 # motorRMOD_I means effecttivly "no retry"
            encRel = 0

        if abs(motorEndPos - motorStartPos) <= abs(self.myBDST) and directionOfMove == directionOfBL:
            while cnt < maxcnt:
                # calculate the delta to move
                # The calculated delta is the scaled, and used for both absolute and relative
                # movements
                delta = motorEndPos - motorStartPos
                if cnt > 1:
                    if rmod == motorRMOD_A:
                        # From motorRecord.cc:
                        #factor = (pmr->rtry - pmr->rcnt + 1.0) / pmr->rtry;
                        factor = 1.0 * (self.myRTRY -  cnt + 1.0) / self.myRTRY
                        delta = delta * factor
                    elif rmod == motorRMOD_G:
                        #factor = 1 / pow(2.0, (pmr->rcnt - 1));
                        rcnt_1 = cnt - 1
                        factor = 1.0
                        while rcnt_1 > 0:
                            factor = factor / 2.0
                            rcnt_1 -= 1
                        delta = delta * factor

                if encRel:
                    line1 = "move relative delta=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                            (delta * frac, self.myBVEL, self.myBAR, motorStartPos)
                else:
                    line1 = "move absolute position=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                            (motorStartPos + delta, self.myBVEL, self.myBAR, motorStartPos)
                expFile.write('%s\n' % (line1))
                cnt += 1
        else:
            # As we don't move the motor (it is simulated, we both times start at motorStartPos
            while cnt < maxcnt:
                # calculate the delta to move
                # The calculated delta is the scaled, and used for both absolute and relative
                # movements
                delta = motorEndPos - motorStartPos - self.myBDST
                if cnt > 1:
                    if rmod == motorRMOD_A:
                        # From motorRecord.cc:
                        #factor = (pmr->rtry - pmr->rcnt + 1.0) / pmr->rtry;
                        factor = 1.0 * (self.myRTRY -  cnt + 1.0) / self.myRTRY
                        delta = delta * factor
                    elif rmod == motorRMOD_G:
                        #factor = 1 / pow(2.0, (pmr->rcnt - 1));
                        rcnt_1 = cnt - 1
                        factor = 1.0
                        while rcnt_1 > 0:
                            factor = factor / 2.0
                            rcnt_1 -= 1
                        delta = delta * factor

                if encRel:
                    line1 = "move relative delta=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                            (delta * frac, self.myVELO, self.myAR, motorStartPos)
                    # Move forward with backlash parameters
                    # Note: This should be self.myBDST, but since we don't move the motor AND
                    # the record uses the readback value, use "motorEndPos - motorStartPos"
                    delta = motorEndPos - motorStartPos
                    line2 = "move relative delta=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                            (delta * frac, self.myBVEL, self.myBAR, motorStartPos)
                else:
                    line1 = "move absolute position=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                            (motorStartPos + delta, self.myVELO, self.myAR, motorStartPos)
                    # Move forward with backlash parameters
                    line2 = "move absolute position=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                            (motorEndPos, self.myBVEL, self.myBAR, motorStartPos)

                expFile.write('%s\n%s\n' % (line1, line2))
                cnt += 1

    def writeExpFileJOG_BDST(self, motor, tc_no, dbgFileName, expFileName, myDirection, frac, encRel, motorStartPos, motorEndPos):
        # Create a "expected" file
        expFile=open(expFileName, 'w')

        # The jogging command
        line1 = "move velocity axis_no=1 direction=%d max_velocity=%g acceleration=%g motorPosNow=%g" % \
                (myDirection, self.myJVEL, self.myJAR, motorStartPos)
        deltaForth = self.myBDST * frac
        # The record tells us to go "delta * frac". Once we have travelled, we are too far
        # The record will read that we are too far, and ask to go back "too far", overcompensated
        # again with frac
        deltaBack = deltaForth * frac
        if encRel:
            # Move back in relative mode
            line2 = "move relative delta=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                    (0 - deltaForth, self.myVELO, self.myAR, motorEndPos)
            # Move relative forward with backlash parameters
            line3 = "move relative delta=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                (deltaBack, self.myBVEL, self.myBAR, motorEndPos - deltaForth)
        else:
            # Move back in positioning mode
            line2 = "move absolute position=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                    (motorEndPos - self.myBDST, self.myVELO, self.myAR, motorEndPos)
            # Move forward with backlash parameters
            line3 = "move absolute position=%g max_velocity=%g acceleration=%g motorPosNow=%g" % \
                (motorEndPos, self.myBVEL, self.myBAR, motorEndPos - self.myBDST)

        expFile.write('%s\n%s\n%s\n' % (line1, line2, line3))
        expFile.close()



    def cmpUnlinkExpectedActualFile(self, dbgFileName, expFileName, actFileName):
        # compare actual and expFile
        sameContent= filecmp.cmp(expFileName, actFileName, shallow=False)
        if not sameContent:
            file = open(expFileName, 'r')
            for line in file:
                if line[-1] == '\n':
                    line = line[0:-1]
                print ("%s: %s" % (expFileName, str(line)));
            file.close();
            file = open(actFileName, 'r')
            for line in file:
                if line[-1] == '\n':
                    line = line[0:-1]
                print ("%s: %s" % (actFileName, str(line)));
            file.close();
            assert(sameContent)
        elif dbgFileName == None:
            unlinkOK = True
            try:
                os.unlink(expFileName)
            except:
                unlinkOK = False
                e = sys.exc_info()
                print str(e)
            try:
                os.unlink(actFileName)
            except:
                unlinkOK = False
                e = sys.exc_info()
                print str(e)
            assert(unlinkOK)

    def setSoftLimitsOff(self, motor):
        """
        Switch off the soft limits
        """
        # switch off the controller soft limits
        try:
            epics.caput(motor + '-CfgDHLM', +3.1e+38, wait=True, timeout=2)
            epics.caput(motor + '-CfgDLLM', -3.1e+38, wait=True, timeout=2)
            epics.caput(motor + '-CfgDHLM-En', 0, wait=True, timeout=2)
            epics.caput(motor + '-CfgDLLM-En', 0, wait=True, timeout=2)
        finally:
            oldRBV = epics.caget(motor + '.RBV')

        if oldRBV > 0:
            epics.caput(motor + '.LLM', 0.0)
            epics.caput(motor + '.HLM', 0.0)
        else:
            epics.caput(motor + '.HLM', 0.0)
            epics.caput(motor + '.LLM', 0.0)

    def setSoftLimitsOn(self, motor, low_limit, high_limit):
        """
        Set the soft limits
        """
        # switch on the controller soft limits
        try:
            epics.caput(motor + '-CfgDHLM', high_limit, wait=True, timeout=2)
            epics.caput(motor + '-CfgDLLM', low_limit,  wait=True, timeout=2)
            epics.caput(motor + '-CfgDHLM-En', 1, wait=True, timeout=2)
            epics.caput(motor + '-CfgDLLM-En', 1, wait=True, timeout=2)
        finally:
            oldRBV = epics.caget(motor + '.RBV')

        if oldRBV < 0:
            epics.caput(motor + '.LLM', low_limit)
            epics.caput(motor + '.HLM', high_limit)
        else:
            epics.caput(motor + '.HLM', high_limit)
            epics.caput(motor + '.LLM', low_limit)


    def doSTUPandSYNC(self, motor, tc_no):
        stup = epics.caget(motor + '.STUP', use_monitor=False)
        while stup != 0:
            stup = epics.caget(motor + '.STUP', use_monitor=False)
            print '%s .STUP=%s' % (tc_no, stup)
            time.sleep(polltime)

        epics.caput(motor + '.STUP', 1)
        epics.caput(motor + '.SYNC', 1)
        rbv = epics.caget(motor + '.RBV', use_monitor=False)
        print '%s .RBV=%f .STUP=%s' % (tc_no, rbv, stup)
        while stup != 0:
            stup = epics.caget(motor + '.STUP', use_monitor=False)
            rbv = epics.caget(motor + '.RBV', use_monitor=False)
            print '%s .RBV=%f .STUP=%s' % (tc_no, rbv, stup)
            time.sleep(polltime)

    def setCNENandWait(self, motor, tc_no, cnen):
        wait_for_power_changed = 6.0
        epics.caput(motor + '-DbgStrToLOG', "CNEN= " + tc_no[0:20]);
        epics.caput(motor + '.CNEN', cnen)
        while wait_for_power_changed > 0:
            msta = int(epics.caget(motor + '.MSTA', use_monitor=False))
            print '%s: wait_for_power_changed=%f msta=%x %s' % (
                tc_no, wait_for_power_changed, msta, self.getMSTAtext(msta))
            if (cnen and (msta & self.MSTA_BIT_AMPON)):
                return True
            if (not cnen and not (msta & self.MSTA_BIT_AMPON)):
                return True
            time.sleep(polltime)
            wait_for_power_changed -= polltime
        return False
