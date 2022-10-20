import json
import serial
import time
from datetime import datetime

# sudo chmod 777 /dev/ttyACM0
serialPort = serial.Serial('COM15', 115200)

# recvJson = json.loads(recvData)

if serialPort .isOpen():
    sendJson = '{"MotorFL":{"speed":111,"direction":1,"time":100},"MotorFR":{"speed":112,"direction":1,"time":100},"MotorRL":{"speed":113,"direction":1,"time":100},"MotorRR":{"speed":114,"direction":1,"time":100}}'
    serialPort .write(bytes(sendJson, 'utf-8'))

    while serialPort .in_waiting == 0:
        continue

    charSerialRead = serialPort .read()
    allReadChars = str("")

    while True:
        if charSerialRead.decode('utf8') == "}" and allReadChars[-1] == "}":
            allReadChars = allReadChars + charSerialRead.decode('utf8')
            break

        allReadChars = allReadChars + charSerialRead.decode('utf8')
        charSerialRead = serialPort .read()

    print(allReadChars)
    y = json.loads(allReadChars)
    print(y)
    serialPort .close()