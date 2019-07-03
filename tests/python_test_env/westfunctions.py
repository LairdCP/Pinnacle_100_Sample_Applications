import os
import subprocess
from constants import *

def WestCompile():
    return subprocess.call("west build -b " + TARGET_TYPE + " -d ./build")

#Using nrfjprog instead
#def WestFlash():
#    return subprocess.call("west flash -b " + TARGET_TYPE + " -d ./build")

def WestCommandFailed(code):
    if (code == 0):
        return false
    else:
        return true
