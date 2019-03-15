#!/usr/bin/env python

# EPICS Single Motion application test script
#
# http://cars9.uchicago.edu/software/python/pyepics3/
#
# m-epics-singlemotion/src/main/test/singlemotion_test.py
# https://nose.readthedocs.org/en/latest/
# https://nose.readthedocs.org/en/latest/testing.html

import epics
import unittest
import os
import sys
import math
import time
from motor_lib import motor_lib
###

def homeTheMotor(self, motor, tc_no, homProce, jogToLSBefore):
    old_high_limit = epics.caget(motor + '.HLM')
    old_low_limit = epics.caget(motor + '.LLM')
    old_HomProc   = self.pv_HomProc.get(use_monitor=False)
    if jogToLSBefore != 0:
        self.lib.setSoftLimitsOff(motor)
        # soft limit range assumed to be = hard range /1.5
        jvel = epics.caget(motor + '.JVEL')
        accl = epics.caget(motor + '.ACCL')
        time_to_wait = 1.5 * (old_high_limit - old_low_limit) / jvel + 2 * accl

        self.lib.jogDirection(motor, tc_no, jogToLSBefore, time_to_wait)
        epics.caput(motor + '.LLM', old_low_limit)
        epics.caput(motor + '.HLM', old_high_limit)
    else:
        self.lib.movePosition(motor, tc_no, (
            old_high_limit + old_low_limit) / 2.0, self.moving_velocity, self.acceleration)

    res = self.pv_HomProc.put(homProce)
    if (res == None):
        print '%s caput -HomProc res=None' % (tc_no)
        self.assertNotEqual(res, None, 'caput -HomProc retuned not None. PV not found ?')

    msta = int(epics.caget(motor + '.MSTA'))
    # We can home while sitting on a limit switch
    if (msta & self.lib.MSTA_BIT_MINUS_LS):
        epics.caput(motor + '.HOMR', 1)
    else:
        epics.caput(motor + '.HOMF', 1)

    time_to_wait = 180
    started = self.lib.waitForStart(motor, tc_no + " homeTheMotor", 3)
    msta1 = int(epics.caget(motor + '.MSTA'))
    if (msta1 & self.lib.MSTA_BIT_HOMED):
      unhomed = 0
    else:
      unhomed = 1
    print '%s homeTheMotor started=%d msta1=%s unhomed=%d' % \
        (tc_no, started, self.lib.getMSTAtext(msta1), unhomed)
    stopped = self.lib.waitForStop(motor, tc_no + " homeTheMotor", time_to_wait)

    msta2 = int(epics.caget(motor + '.MSTA'))
    homed = 0
    if (msta2 & self.lib.MSTA_BIT_HOMED):
        homed = 1
    print '%s homeTheMotor stopped=%d msta2=%s homed=%d' % \
        (tc_no, stopped, self.lib.getMSTAtext(msta2), homed)
    #self.assertEqual(True, started,                          tc_no +  "started = True")
    self.assertEqual(True, stopped,                          tc_no +  "stopped = True")
    self.assertEqual(0, msta2 & self.lib.MSTA_BIT_SLIP_STALL, tc_no + "MSTA.no MSTA_BIT_SLIP_STALL")
    self.assertNotEqual(0, homed,   tc_no + "MSTA.homed (Axis has been homed)")
    self.pv_HomProc.put(old_HomProc, wait=True)


def homeLimBwdfromLLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 1, -1)

def homeLimBwdfromMiddle(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 1, 0)

def homeLimBwdfromHLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 1, 1)


def homeLimFwdfromLLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 2, -1)

def homeLimFwdfromMiddle(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 2, 0)

def homeLimFwdfromHLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 2, 1)


def homeSwitchfromLimBwdFromLLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 3, -1)

def homeSwitchfromLimBwdFromHLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 3, 1)

def homeSwitchfromLimBwdFromMiddle(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 3, 0)


def homeSwitchfromLimFwdFromLLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 4, -1)

def homeSwitchfromLimFwdFromHLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 4, 1)

def homeSwitchfromLimFwdFromMiddle(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 4, 0)

def homeSwitchMidfromLimBwdFromLLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 5, -1)

def homeSwitchMidfromLimBwdFromHLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 5, 1)

def homeSwitchMidfromLimBwdFromMiddle(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 5, 0)


def homeSwitchMidfromLimFwdFromLLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 6, -1)

def homeSwitchMidfromLimFwdFromHLS(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 6, 1)

def homeSwitchMidfromLimFwdFromMiddle(self, tc_no):
    homeTheMotor(self, self.m1, tc_no, 6, 0)


class Test(unittest.TestCase):
    lib = motor_lib()
    m1 = os.getenv("TESTEDMOTORAXIS")
    pv_HomProc = epics.PV(os.getenv("TESTEDMOTORAXIS") + "-HomProc")
    res = pv_HomProc.get()
    if (res == None):
        print 'caget -HomProc res=None'
        pv_HomProc = None
    else:
        old_HomProc   = pv_HomProc.get(use_monitor=False)

    acceleration     = epics.caget(m1 + '.ACCL')
    moving_velocity  = epics.caget(m1 + '.VELO')

    def test_TC_11100(self):
        tc_no = "TC-11100"
        print '%s Home ' % tc_no
        self.assertNotEqual(self.pv_HomProc, None, '-HomProc not found')

    def test_TC_11110(self):
        tc_no = "TC-11110"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeLimBwdfromLLS(self, tc_no)

    def test_TC_11111(self):
        tc_no = "TC-11111"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeLimBwdfromMiddle(self, tc_no)

    def test_TC_11112(self):
        tc_no = "TC-11112"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeLimFwdfromLLS(self, tc_no)

    def test_TC_11120(self):
        tc_no = "TC-11120"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeLimFwdfromMiddle(self, tc_no)

    def test_TC_11121(self):
        tc_no = "TC-11121"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeLimFwdfromHLS(self, tc_no)

    def test_TC_11122(self):
        tc_no = "TC-11122"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeLimBwdfromHLS(self, tc_no)

    def test_TC_11130(self):
        tc_no = "TC-11130"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeSwitchfromLimFwdFromLLS(self, tc_no)

    def test_TC_11131(self):
        tc_no = "TC-11131"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeSwitchfromLimFwdFromMiddle(self, tc_no)

    def test_TC_11132(self):
        tc_no = "TC-11132"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeSwitchfromLimBwdFromLLS(self, tc_no)

    def test_TC_11140(self):
        tc_no = "TC-11140"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeSwitchfromLimBwdFromMiddle(self, tc_no)

    def test_TC_11141(self):
        tc_no = "TC-11141"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeSwitchfromLimBwdFromHLS(self, tc_no)

    def test_TC_11142(self):
        tc_no = "TC-11142"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeSwitchfromLimFwdFromHLS(self, tc_no)

#    def test_TC_11150(self):
#        tc_no = "TC-11150"
#        print '%s Home ' % tc_no
#        if self.pv_HomProc != None:
#            homeSwitchMidfromLimBwdFromMiddle(self, tc_no)
#
#    def test_TC_11151(self):
#        tc_no = "TC-11151"
#        print '%s Home ' % tc_no
#        if self.pv_HomProc != None:
#            homeSwitchMidfromLimBwdFromHLS(self, tc_no)
#
#    def test_TC_11152(self):
#        tc_no = "TC-11152"
#        print '%s Home ' % tc_no
#        if self.pv_HomProc != None:
#            homeSwitchMidfromLimFwdFromHLS(self, tc_no)
#
#    def test_TC_11160(self):
#        tc_no = "TC-11160"
#        print '%s Home ' % tc_no
#        homeSwitchMidfromLimFwdFromLLS(self, tc_no)
#
#    def test_TC_11161(self):
#        tc_no = "TC-11161"
#        print '%s Home ' % tc_no
#        if self.pv_HomProc != None:
#            homeSwitchMidfromLimFwdFromMiddle(self, tc_no)
#
#    def test_TC_11162(self):
#        tc_no = "TC-11162"
#        print '%s Home ' % tc_no
#        if self.pv_HomProc != None:
#            homeSwitchMidfromLimBwdFromLLS(self, tc_no)

    # Need to home with the original homing procedure
    def test_TC_11191(self):
        tc_no = "TC-11191"
        print '%s Home ' % tc_no
        if self.pv_HomProc != None:
            homeTheMotor(self, self.m1, tc_no, self.old_HomProc, 0)


