import epics

motor1 = 'PSI-ESTIARND:MC-MCU-01:m1'

def init(motor):
    epics.caput(motor + '.MISV', 2)
    epics.caput(motor + '.HLSV', 2)


def movabs(motor, position):
    epics.caput(motor, position, wait=True)
    stat = epics.caget(motor + '.STAT', use_monitor=False)
    if stat != 0:
        rbv = epics.caget(motor + '.RBV', use_monitor=False)
        msg = "ERROR: " + motor + " .STAT " + str(stat) + ' position=' + str(position) + ' RBV=' + str(rbv)
        raise Exception(__name__ + msg)


movabs(motor1, 15.0)
movabs(motor1, 60.0)
