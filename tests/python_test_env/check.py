import socket
import ssl
import urllib.request
import urllib.parse
import subprocess
import os
from constants import *
from serialfunctions import *
from westfunctions import *
from nordicfunctions import *
from pathlib import Path

DATA_POST = "Lorem ipsum dolor sit amet, consectetur adipiscing elit,sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna idvolutpat lacus laoreet non. Elit pellentesque habitant morbi tristiquesenectus et. Penatibus et magnis dis parturient montes nascetur ridiculusmus mauris. Arcu non sodales neque sodales ut etiam sit. Amet venenatisurna cursus eget nunc. In mollis nunc sed id semper risus in hendrerit.Tristique senectus et netus et. Mus mauris vitae ultricies leo integermalesuada nunc. Ut ornare lectus sit amet est placerat in egestas erat.Blandit turpis cursus in hac habitasse platea dictumst. Dui nunc mattis"

DATA_1 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer vestibulum, ligula et venenatis pulvinar, odio augue rhoncus mi, viverra euismod felis dolor at erat. Pellentesque purus ex, ultricies non accumsan a, tempus eget enim. Aliquam molestie, tortor in varius fermentum, purus sapien suscipit enim, id lobortis turpis dui sed sem. Vestibulum ut sem tempor elit tristique porttitor. Ut id pulvinar lacus, vel pellentesque velit. Donec auctor sed sem et placerat. Cras eu tempor neque. Donec tincidunt sed leo quis consequat. Quisque pellentesque est mi, semper imperdiet mauris volutpat et. Sed nisi arcu, lacinia in odio sed, lobortis placerat libero. Nulla ac augue tristique, aliquet metus id, convallis lectus. Etiam suscipit ante vitae sem ornare imperdiet. Nullam hendrerit sapien a ligula posuere, et semper lorem volutpat. Morbi iaculis diam eu bibendum feugiat."
DATA_2 = "Vestibulum scelerisque libero a est vestibulum, vel consequat leo commodo. Nunc vulputate neque magna. Praesent commodo ipsum at nisl tempor pretium. Sed imperdiet cursus mi ut varius. Aenean egestas lectus sem, sed tempus massa pellentesque laoreet. Praesent in quam velit. Ut pellentesque, lorem eget euismod volutpat, sapien ex porttitor nunc, quis ultricies enim magna ac ex. Aliquam laoreet felis sem, sed laoreet justo vehicula nec. Aliquam augue arcu, posuere eu sagittis non, porttitor quis eros. Maecenas eget orci dolor. Pellentesque ultricies dignissim metus auctor faucibus. Nunc nec imperdiet eros, at eleifend mauris. Aliquam vestibulum ultricies ligula vel tincidunt."
DATA_3 = "In pretium erat est, gravida placerat lorem vulputate eget. Duis nisl nunc, posuere nec tempor vel, pretium et eros. Nunc suscipit eget sem at lacinia. Curabitur neque tellus, dictum eget fringilla at, molestie hendrerit nulla. Nunc ut ante vitae tellus auctor fringilla. Phasellus in enim id eros pulvinar eleifend in fringilla neque. Proin ullamcorper in nulla a accumsan. Nulla malesuada orci et eros consectetur congue vitae in massa. Vivamus sed augue eget neque sodales consequat id et metus. Quisque in ligula dolor. Pellentesque nec rutrum eros, sit amet luctus ligula."
DATA_4 = "Morbi lacus turpis, finibus sed mattis blandit, consequat vitae est. Donec at blandit massa. Praesent aliquet malesuada tellus non placerat. Sed at tellus nibh. Vestibulum vitae magna ut tortor condimentum fringilla. Cras laoreet tortor semper sapien cursus, vitae venenatis ex dignissim. Donec eleifend erat metus, vitae consectetur lectus pretium a. Praesent sit amet eleifend arcu, sed consequat est. Integer eget risus elit. Nullam posuere id lorem et pellentesque."
DATA_5 = "In hac habitasse platea dictumst. Curabitur rhoncus vitae sem non suscipit. Mauris blandit lorem et mi iaculis congue. Sed sed felis quis lorem vehicula dictum sit amet nec eros. Ut imperdiet lacus nec ullamcorper viverra. Vivamus nec pulvinar dolor, ut consectetur nibh. Maecenas ut cursus dui. Donec porta felis blandit felis dictum tempor vel a mauris. Aliquam erat volutpat. Nunc vel metus et felis lobortis hendrerit. Pellentesque vel aliquet lorem. In tincidunt elit porta justo volutpat, tincidunt sollicitudin massa semper. Morbi facilisis, nisi in accumsan vulputate, libero mi condimentum diam, in pellentesque justo arcu quis nisl."
DATA_6 = "Interdum et malesuada fames ac ante ipsum primis in faucibus. Nullam tempor, justo vitae euismod feugiat, quam lorem facilisis massa, et mollis leo nulla sed neque. Nunc sit amet ipsum porta lectus ullamcorper mollis. Morbi malesuada faucibus lectus et lacinia. Vivamus et dui in turpis mattis dignissim. Mauris non elit ut felis ornare ultrices. Vestibulum dignissim quam mauris, non porta felis consectetur ut. Sed sapien velit, vulputate ut blandit vitae, finibus sed mi. Proin arcu tellus, rutrum nec varius vel, elementum vitae tellus. Fusce imperdiet vulputate blandit. Aliquam libero urna, feugiat in augue eu, porttitor congue mi. Ut eget orci quis orci volutpat commodo. Nunc tristique tortor nibh, luctus tincidunt nulla faucibus sed. Nulla ut placerat quam. Nam cursus, neque et tempus consequat, ipsum lorem tempus lacus, ac ultrices libero est ac tortor. Donec ut ex pretium, aliquet lorem id, lobortis est."
DATA_7 = "In sit amet neque lectus. Pellentesque id nisl in diam convallis iaculis eu quis nulla. Aliquam erat volutpat. Donec vulputate ultricies est ut venenatis. Morbi ac lacinia diam. Pellentesque ut laoreet enim, id laoreet eros. Proin sit amet ligula eu felis bibendum eleifend in vitae elit. Quisque vehicula, mi eu maximus accumsan, risus velit eleifend quam, fringilla elementum tortor tortor at mauris. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Ut tempor sollicitudin metus in auctor."
DATA_8 = "Donec dolor risus, faucibus vitae nisi mollis, porta egestas nulla. Suspendisse nec massa faucibus, venenatis tellus eleifend, facilisis urna. Nunc porttitor neque ac lectus convallis, eget bibendum lectus hendrerit. Duis tincidunt commodo hendrerit. Vestibulum suscipit ornare justo non lobortis. Ut ornare luctus tempus. Quisque sed rhoncus neque. Nullam maximus lectus in sem elementum, sollicitudin ornare nunc pellentesque. Nunc interdum felis non magna tempor, eu tempus sem venenatis. Nullam eget urna gravida, aliquam lacus dictum, suscipit tellus."

def SetupSSL():
    SSLContext.load_verify_locations(HTTPS_CERTIFICATE)

def ClearData():
    urllib.request.urlopen("http://" + HTTP_HOSTNAME + "/" + HTTP_PATH + "/" + HTTP_CLEAR, context=SSLContext)

def AddData(Data):
    urllib.request.urlopen("http://" + HTTP_HOSTNAME + "/" + HTTP_PATH + "/" + HTTP_POST + "?" + HTTP_POST_VAR + "=" + urllib.parse.quote(Data, safe=''), context=SSLContext)

def GetData():
    return urllib.request.urlopen("http://" + HTTP_HOSTNAME + "/" + HTTP_PATH + "/" + HTTP_LOGS, context=SSLContext).read()

#Calculate paths
file_path = os.path.dirname(os.path.realpath(__file__))
main_path = Path(file_path + MAIN_DIRECTORY)
zephyr_path = Path(str(main_path) + ZEPHYR_DIRECTORY)
tests_path = Path(str(main_path) + TEST_SUBDIRECTORY)

#Add zephyr base environment variable
os.environ["ZEPHYR_BASE"] = str(zephyr_path)

#Setup SSL context
SSLContext = ssl.create_default_context()

#Set nordic device
NordicSetDevice(TEST_BOARD_1_SEGGER_ID)

#Erase module
NordicEraseAll()

#Set SSL context up
SetupSSL()

#Clear all data
ClearData()

#Check data is empty
wsData = GetData()
if (len(wsData) != 0):
    raise ValueError("Data list should be empty but isn't")

#Open serial port
OpenSerial(TEST_BOARD_1_SERIAL_PORT)

#Change to test directory of first test
os.chdir(Path(str(tests_path) + "/http_get_query"))

#Compile test
westcode = WestCompile()

#Flash test
#westcode = WestFlash()
NordicProgram(str(Path("build/zephyr/zephyr.hex")))
NordicReset()
print("done")

#Wait for events
RecDat = ""
Timeout = 0
Finished = False
Index = 0

while (Timeout < 100 and Finished == False):
    RecDat = RecDat + SerialRead(100).decode("utf-8")
    Needle = ""
    if (Index == 0):
        Needle = "modem_hl7800: Resetting Modem"
    elif (Index == 1):
        Needle = "modem_hl7800: Keep awake"
    elif (Index == 2):
        Needle = "modem_hl7800: Manufacturer: Sierra Wireless"
    elif (Index == 3):
        Needle = "modem_hl7800: Model: HL7800"
    elif (Index == 4):
        Needle = "modem_hl7800: Modem ready!"
    elif (Index == 5):
        Needle = "http_get: Starting tests..."
    elif (Index == 6):
        Needle = "http_get: Run http_get_query #1"
    elif (Index == 7):
        Needle = "Response:"
    elif (Index == 8):
        Needle = "HTTP/1.1 200 OK"
    elif (Index == 9):
        Needle = "OK"
    elif (Index == 10):
        Needle = "http_get: Run http_get_query #2"
    elif (Index == 11):
        Needle = "Response:"
    elif (Index == 12):
        Needle = "HTTP/1.1 200 OK"
    elif (Index == 13):
        Needle = "OK"
    elif (Index == 14):
        Needle = "http_get: Run http_get_query #3"
    elif (Index == 15):
        Needle = "Response:"
    elif (Index == 16):
        Needle = "HTTP/1.1 200 OK"
    elif (Index == 17):
        Needle = "OK"
    elif (Index == 18):
        Needle = "http_get: Run http_get_query #4"
    elif (Index == 19):
        Needle = "Response:"
    elif (Index == 20):
        Needle = "HTTP/1.1 200 OK"
    elif (Index == 21):
        Needle = "OK"
#    elif (Index == 22):
#        Needle = "http_get: Run http_get_query #5"
#    elif (Index == 23):
#        Needle = "Response:"
#    elif (Index == 24):
#        Needle = "HTTP/1.1 200 OK"
#    elif (Index == 25):
#        Needle = "OK"
#    elif (Index == 26):
#        Needle = "http_get: Run http_get_query #6"
#    elif (Index == 27):
#        Needle = "Response:"
#    elif (Index == 28):
#        Needle = "HTTP/1.1 200 OK"
#    elif (Index == 29):
#        Needle = "OK"
#    elif (Index == 30):
#        Needle = "http_get: Run http_get_query #7"
#    elif (Index == 31):
#        Needle = "Response:"
#    elif (Index == 32):
#        Needle = "HTTP/1.1 200 OK"
#    elif (Index == 33):
#        Needle = "OK"
#    elif (Index == 34):
#        Needle = "http_get: Run http_get_query #8"
#    elif (Index == 35):
#        Needle = "Response:"
#    elif (Index == 36):
#        Needle = "HTTP/1.1 200 OK"
#    elif (Index == 37):
#        Needle = "OK"
#    elif (Index == 38):
    elif (Index == 22):
        Needle = "blahblah"
        Finished = True

    StrPos = RecDat.find(Needle)
    if (StrPos != -1):
        Index = Index + 1
        print("Next search, search for #" + str(Index) + "...")
        StrPos = RecDat.find("\r\n", StrPos)
        if (StrPos == -1):
            StrPos = RecDat.find(Needle)
        else:
            StrPos = StrPos + 2
        RecDat = RecDat[StrPos:]

    Timeout = Timeout + 1
print("First test ended, test finished: " + str(Finished))

#Check data is present
wsData = str(GetData())
if (len(wsData) == 0):
    raise ValueError("Data list should contain data but is empty")
StrPos = wsData.find(DATA_POST)
if (StrPos == -1):
    raise ValueError("Missing entry #1")
StrPos = wsData.find(DATA_POST, StrPos + 2)
if (StrPos == -1):
    raise ValueError("Missing entry #2")
StrPos = wsData.find(DATA_POST, StrPos + 2)
if (StrPos == -1):
    raise ValueError("Missing entry #3")
StrPos = wsData.find(DATA_POST, StrPos + 2)
if (StrPos == -1):
    raise ValueError("Missing entry #4")
#StrPos = wsData.find(DATA_POST, StrPos + 2)
#if (StrPos == -1):
#    raise ValueError("Missing entry #5")
#StrPos = wsData.find(DATA_POST, StrPos + 2)
#if (StrPos == -1):
#    raise ValueError("Missing entry #6")
#StrPos = wsData.find(DATA_POST, StrPos + 2)
#if (StrPos == -1):
#    raise ValueError("Missing entry #7")
#StrPos = wsData.find(DATA_POST, StrPos + 2)
#if (StrPos == -1):
#    raise ValueError("Missing entry #8")

#Clear all data
ClearData()

#Add data to system
AddData(DATA_1)
AddData(DATA_2)
AddData(DATA_3)
AddData(DATA_4)
AddData(DATA_5)
AddData(DATA_6)
AddData(DATA_7)
AddData(DATA_8)

#Change to test directory of second test
os.chdir(Path(str(tests_path) + "/http_get_query_check"))

#Compile test
westcode = WestCompile()

#Erase module
NordicEraseAll()

#Flash test
NordicProgram(str(Path("build/zephyr/zephyr.hex")))
NordicReset()
print("done")

#Wait for events
RecDat = ""
Timeout = 0
Finished = False
Index = 0

while (Timeout < 100 and Finished == False):
    RecDat = RecDat + SerialRead(100).decode("utf-8")
    Needle = ""
    if (Index == 0):
        Needle = "modem_hl7800: Resetting Modem"
    elif (Index == 1):
        Needle = "modem_hl7800: Keep awake"
    elif (Index == 2):
        Needle = "modem_hl7800: Manufacturer: Sierra Wireless"
    elif (Index == 3):
        Needle = "modem_hl7800: Model: HL7800"
    elif (Index == 4):
        Needle = "modem_hl7800: Modem ready!"
    elif (Index == 5):
        Needle = "http_get: Starting tests..."
    elif (Index == 6):
        Needle = "http_get: Run http_get_query_check #1"
    elif (Index == 7):
        Needle = "Response:"
    elif (Index == 8):
        Needle = "HTTP/1.1 200 OK"
    elif (Index == 9):
        Needle = DATA_1
    elif (Index == 10):
        Needle = DATA_2
    elif (Index == 11):
        Needle = DATA_3
    elif (Index == 12):
        Needle = DATA_4
    elif (Index == 13):
        Needle = DATA_5
    elif (Index == 14):
        Needle = DATA_6
    elif (Index == 15):
        Needle = DATA_7
    elif (Index == 16):
        Needle = DATA_8
    elif (Index == 17):
        Needle = "blahblah"
        Finished = True

    StrPos = RecDat.find(Needle)
    if (StrPos != -1):
        Index = Index + 1
        print("Next search, search for #" + str(Index) + "...")
        StrPos = RecDat.find("\r\n", StrPos)
        if (StrPos == -1):
            StrPos = RecDat.find(Needle)
        else:
            StrPos = StrPos + 2
        RecDat = RecDat[StrPos:]

    Timeout = Timeout + 1
print("Second test ended, test finished: " + str(Finished))


#Check all data is present
#wsData = GetData()
#if (len(wsData) == 0):
#    raise ValueError("Data list should contain data but is empty")
#elif (len(wsData) < (len(DATA_1) + len(DATA_2) + len(DATA_3) + len(DATA_4) + len(DATA_5) + len(DATA_6) + len(DATA_7) + len(DATA_8))):
#    raise ValueError("Data list should contain data but contains insufficent data")

#Check each line is correct
#wsData = wsData.split(b'\r\n')
#i = 0
#while (i < len(wsData)-1):
#    thisData = str(wsData[i].split(b': ')[1].decode("utf-8"))
#    print("check...\r\n")
#    if ((i == 0 and thisData != DATA_1) or (i == 1 and thisData != DATA_2) or (i == 2 and thisData != DATA_3) or (i == 3 and thisData != DATA_4) or (i == 4 and thisData != DATA_5) or (i == 5 and thisData != DATA_6) or (i == 6 and thisData != DATA_7) or (i == 7 and thisData != DATA_8)):
#        raise ValueError("Data #" + str(i) + " mismatch")
#    i += 1
##bob = wsData[0].split(b': ')
###print(wsData[0])
##print(bob[1])

##len(DATA_1) + len(DATA_2) + len(DATA_3) + len(DATA_4) + len(DATA_5) + len(DATA_6) + len(DATA_7) + len(DATA_8)

#Close serial port
CloseSerial()
