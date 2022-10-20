import socket
import json
import serial

# Server port
port = 4444


def get_ip(): # Function returns IPv4 address
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        s.connect(('10.254.254.254', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP


# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# Bind the socket to the address given on the command line
server_address = (get_ip(), port)
sock.bind(server_address)
print('Server address: %s, Port: %s' % sock.getsockname())
# Setting limit on client connections
sock.listen(2)

# Read/write permissions are need to write/read from serial port
# sudo chmod 777 /dev/ttyACM0
serialPort = serial.Serial('/dev/ttyACM0', 115200) # Port name and baud rate

while sock:
    print('Waiting for a connection')
    connection, client_address = sock.accept()

    if not serialPort.isOpen():
        serialPort.open()

    try:
        print('Client connected: ', client_address)
        while True:
            recvData = connection.recv(256)
            #recvJson = json.loads(recvData)

            if len(recvData) > 0:
                print('Received "%s"' % recvData.decode('utf8'))
                # Checking if serial port is open
                if serialPort.isOpen():
                    # sendJson = '{"MotorFL":{"speed":111,"direction":1,"time":100},"MotorFR":{"speed":112,"direction":1,"time":100},"MotorRL":{"speed":113,"direction":1,"time":100},"MotorRR":{"speed":114,"direction":1,"time":100}}'
                    serialPort.write(recvData)
                    print('Serial data send')
                    # Waiting for Arduino response (data on serial port)
                    while serialPort.in_waiting == 0:
                        continue

                    # Creating string for read JSON data
                    allReadChars = str("")
                    # Reading first byte
                    charSerialRead = serialPort.read()
                    # Reading serial dat until 2x'}' (JSON frame end)
                    while True:
                        # Adding read byte to string
                        allReadChars = allReadChars + charSerialRead.decode('utf8')

                        # Checking for two '}' in last two chars
                        if len(allReadChars) > 10 and allReadChars[-2] == "}" and allReadChars[-1] == "}":
                            break
                        #Reading next byte
                        charSerialRead = serialPort.read()
                    print('Serial data received')
                else:
                    charSerialRead = '{"Serial": "closed"}'
                # Sending received data to client in JSON
                connection.sendall(allReadChars.encode())
                print('Serial response send to client')
                print(allReadChars)
            else:
                break
    finally:
        connection.close()
        if serialPort.isOpen():
            serialPort.close()

