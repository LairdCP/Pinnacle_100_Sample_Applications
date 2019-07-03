import os
import subprocess
from constants import *

NordicSerial = None

def NordicSetDevice(SerialNumber):
    global NordicSerial
    NordicSerial = SerialNumber

def NordicProgram(HexFile):
    return subprocess.call("nrfjprog -f NRF52 --snr " + NordicSerial + " --program " + HexFile)

def NordicEraseFlash():
    return subprocess.call("nrfjprog -f NRF52 --snr " + NordicSerial + " --eraseall")

def NordicEraseQSPIHeader():
    return subprocess.call("nrfjprog -f NRF52 --snr " + NordicSerial + " --erasepage " + str(QSPI_HEADER_START_ADDRESS) + "-" + str(QSPI_HEADER_END_ADDRESS))

def NordicEraseAll():
    NordicEraseFlash()
    NordicEraseQSPIHeader()

def NordicReset():
    return subprocess.call("nrfjprog -f NRF52 --snr " + NordicSerial + " --reset")
