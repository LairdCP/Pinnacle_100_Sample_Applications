import serial
import io

ser = None
seropen = False

def OpenSerial(Port):
    global ser
    global seropen
    ser = serial.Serial(Port, baudrate=115200, timeout=1, write_timeout=1, rtscts=True)
    seropen = True

def IsSerialOpen():
    return seropen

def CloseSerial():
    global ser
    global seropen
    ser.close()
    ser = None
    seropen = False

def SerialWrite(Data):
    ser.write(Data)
    ser.flush()

def SerialRead(MaxSize):
    return ser.read(MaxSize)
