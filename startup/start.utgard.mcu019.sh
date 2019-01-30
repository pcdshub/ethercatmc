#!/bin/bash

source /epics/base-3.15.5/require/3.0.4/bin/setE3Env.bash

/usr/bin/procServ -f -L /var/log/procServ/labs-utgard-mcu019 -i ^C^D -c /var/run/procServ/labs-utgard-mcu019 2100 /epics/base-3.15.5/require/3.0.4/bin/iocsh.bash /epics/iocs/cmds/labs-utgard-mcu019/m-epics-ethercatmc/startup/st.utgard.mcu019.cmd
