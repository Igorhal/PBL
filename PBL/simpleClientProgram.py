import socket

# Destination server port
port = 4444

# Creating server socket, assigning address and port
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('192.168.8.103', port)
print('Connecting to address: %s, Port: %s' % server_address)
sock.connect(server_address)

try:
    # Send data
    instructionJson = '{"MotorFL":{"speed":111,"direction":1,"time":100},"MotorFR":{"speed":112,"direction":1,"time":100},"MotorRL":{"speed":113,"direction":1,"time":100},"MotorRR":{"speed":114,"direction":1,"time":100}}'

    print(instructionJson)
    sock.send(instructionJson.encode())

    # Reading server response, until 2x}
    while True:
        recvData = str(sock.recv(512))
        print(len(recvData))
        if len(recvData) > 10 and recvData[-3] == '}' and recvData[-2] == '}':
            break

    print(recvData)

finally:
    print('Closing socket!')
    sock.close()


