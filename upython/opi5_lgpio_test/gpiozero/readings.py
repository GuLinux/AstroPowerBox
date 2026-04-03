#!/usr/bin/env python3

import os
import time
import ADS1x15

#!/usr/bin/env python
from ina219 import INA219
from ina219 import DeviceRangeError

SHUNT_OHMS = 0.4

ina = INA219(SHUNT_OHMS, busnum=2)
ina.configure()

def read_ina():

    print("Bus Voltage: %.3f V" % ina.voltage())
    try:
        print("Bus Current: %.3f mA" % ina.current())
        print("Power: %.3f mW" % ina.power())
        print("Shunt voltage: %.3f mV" % ina.shunt_voltage())
    except DeviceRangeError as e:
        # Current out of device range with specified shunt resistor
        print(e)



ADS = ADS1x15.ADS1115(2, 0x48)

print(os.path.basename(__file__))
print("ADS1X15_LIB_VERSION: {}".format(ADS1x15.__version__))

# set gain to 4.096V max
ADS.setGain(ADS.PGA_4_096V)
f = ADS.toVoltage()

while True :
    val_0 = ADS.readADC(0)
    val_1 = ADS.readADC(1)
    val_2 = ADS.readADC(2)
    val_3 = ADS.readADC(3)
    print("Analog0: {0:d}\t{1:.3f} V".format(val_0, val_0 * f))
    print("Analog1: {0:d}\t{1:.3f} V".format(val_1, val_1 * f))
    print("Power: %.3f W" % (ina.power()/1000.0))
    print('')
    #print("Analog2: {0:d}\t{1:.3f} V".format(val_2, val_2 * f))
    #print("Analog3: {0:d}\t{1:.3f} V".format(val_3, val_3 * f))
    time.sleep(1)
