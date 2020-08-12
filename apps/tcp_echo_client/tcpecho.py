#! /usr/bin/env python3

# Server for TCP echo.
#
# Usage: tcpecho -s [port]	(to start a server)

import sys
import signal
import socket

ECHO_PORT = 50000 + 7
BUFSIZE = 10240
SOCKET_TIMEOUT_SECS = 30
conn = 0
s = 0
bigData = ("Lorem ipsum dolor sit amet, consectetur adipiscing elit,\n"
           "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna id\n"
           "volutpat lacus laoreet non. Elit pellentesque habitant morbi tristique\n"
           "senectus et. Penatibus et magnis dis parturient montes nascetur ridiculus\n"
           "mus mauris. Arcu non sodales neque sodales ut etiam sit. Amet venenatis\n"
           "urna cursus eget nunc. In mollis nunc sed id semper risus in hendrerit.\n"
           "Tristique senectus et netus et. Mus mauris vitae ultricies leo integer\n"
           "malesuada nunc. Ut ornare lectus sit amet est placerat in egestas erat.\n"
           "Blandit turpis cursus in hac habitasse platea dictumst. Dui nunc mattis\n"
           "enim ut tellus elementum sagittis. Molestie nunc non blandit massa enim\n"
           "nec. Sed viverra tellus in hac habitasse platea dictumst vestibulum.\n"
           "Eget est lorem ipsum dolor sit amet.\n"
           "Auctor augue mauris augue neque gravida in fermentum. Nulla pellentesque\n"
           "dignissim enim sit amet. Pharetra magna ac placerat vestibulum lectus mauris\n"
           "ultrices eros in. Volutpat lacus laoreet non curabitur. Neque vitae tempus\n"
           "quam pellentesque nec nam. Semper eget duis at tellus at urna condimentum\n"
           "mattis pellentesque. Viverra maecenas accumsan lacus vel facilisis volutpat\n"
           "est. Tristique risus nec feugiat in fermentum posuere urna nec. Amet commodo\n"
           "nulla facilisi nullam vehicula ipsum a arcu cursus. Gravida dictum fusce ut\n"
           "placerat orci nulla pellentesque. Diam vulputate ut pharetra sit amet aliquam\n"
           "id diam maecenas. Aliquam vestibulum morbi blandit cursus risus. Blandit\n"
           "turpis cursus in hac habitasse platea dictumst quisque sagittis.\n"
           "Adipiscing bibendum est ultricies integer quis.\n"
           "Sed faucibus turpis in eu mi. Viverra maecenas accumsan lacus vel facilisis\n"
           "volutpat est velit egestas. Egestas tellus rutrum tellus pellentesque eu.\n"
           "Et malesuada fames ac turpis egestas integer. Elit sed vulputate mi sit\n"
           "amet mauris commodo quis imperdiet. Mus mauris vitae ultricies leo integer\n"
           "malesuada nunc vel. Leo vel orci porta non pulvinar. Laoreet non curabitur\n"
           "gravida arcu ac tortor dignissim convallis aenean. Dictum varius duis at\n"
           "consectetur lorem. Tempus imperdiet nulla malesuada pellentesque. Tellus\n"
           "rutrum tellus pellentesque eu tincidunt tortor. Vel elit scelerisque mauris\n"
           "pellentesque pulvinar pellentesque habitant morbi. Sollicitudin nibh sit amet\n"
           "commodo nulla facilisi nullam vehicula. Vel pretium lectus quam id leo in\n"
           "vitae. Dolor sit amet consectetur adipiscing elit ut aliquam purus sit.\n"
           "Cras semper auctor neque vitae tempus quam pellentesque. Aliquam ultrices\n"
           "sagittis orci a scelerisque. Amet porttitor eget dolor morbi.\n\n"
           "   Lorem ipsum dolor sit amet, consectetur adipiscing elit,\n"
           "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna id\n"
           "volutpat lacus laoreet non. Elit pellentesque habitant morbi tristique\n"
           "senectus et. Penatibus et magnis dis parturient montes nascetur ridiculus\n"
           "mus mauris. Arcu non sodales neque sodales ut etiam sit. Amet venenatis\n"
           "urna cursus eget nunc. In mollis nunc sed id semper risus in hendrerit.\n"
           "Tristique senectus et netus et. Mus mauris vitae ultricies leo integer\n"
           "malesuada nunc. Ut ornare lectus sit amet est placerat in egestas erat.\n"
           "Blandit turpis cursus in hac habitasse platea dictumst. Dui nunc mattis\n"
           "enim ut tellus elementum sagittis. Molestie nunc non blandit massa enim\n"
           "nec. Sed viverra tellus in hac habitasse platea dictumst vestibulum.\n"
           "Eget est lorem ipsum dolor sit amet.\n"
           "Auctor augue mauris augue neque gravida in fermentum. Nulla pellentesque\n"
           "dignissim enim sit amet. Pharetra magna ac placerat vestibulum lectus mauris\n"
           "ultrices eros in. Volutpat lacus laoreet non curabitur. Neque vitae tempus\n"
           "quam pellentesque nec nam. Semper eget duis at tellus at urna condimentum\n"
           "mattis pellentesque. Viverra maecenas accumsan lacus vel facilisis volutpat\n"
           "est. Tristique risus nec feugiat in fermentum posuere urna nec. Amet commodo\n"
           "nulla facilisi nullam vehicula ipsum a arcu cursus. Gravida dictum fusce ut\n"
           "placerat orci nulla pellentesque. Diam vulputate ut pharetra sit amet aliquam\n"
           "id diam maecenas. Aliquam vestibulum morbi blandit cursus risus. Blandit\n"
           "turpis cursus in hac habitasse platea dictumst quisque sagittis.\n"
           "Adipiscing bibendum est ultricies integer quis.\n"
           "Sed faucibus turpis in eu mi. Viverra maecenas accumsan lacus vel facilisis\n"
           "volutpat est velit egestas. Egestas tellus rutrum tellus pellentesque eu.\n"
           "Et malesuada fames ac turpis egestas integer. Elit sed vulputate mi sit\n"
           "amet mauris commodo quis imperdiet. Mus mauris vitae ultricies leo integer\n"
           "malesuada nunc vel. Leo vel orci porta non pulvinar. Laoreet non curabitur\n"
           "gravida arcu ac tortor dignissim convallis aenean. Dictum varius duis at\n"
           "consectetur lorem. Tempus imperdiet nulla malesuada pellentesque. Tellus\n"
           "rutrum tellus pellentesque eu tincidunt tortor. Vel elit scelerisque mauris\n"
           "pellentesque pulvinar pellentesque habitant morbi. Sollicitudin nibh sit amet\n"
           "commodo nulla facilisi nullam vehicula. Vel pretium lectus quam id leo in\n"
           "vitae. Dolor sit amet consectetur adipiscing elit ut aliquam purus sit.\n"
           "Cras semper auctor neque vitae tempus quam pellentesque. Aliquam ultrices\n"
           "sagittis orci a scelerisque. Amet porttitor eget dolor morbi.")


def sig_handling(signum, frame):
    global conn, s
    try:
        conn.close()
    except:
        pass
    finally:
        s.close()
        sys.exit()


signal.signal(signal.SIGINT, sig_handling)


def main():
    if len(sys.argv) < 2:
        usage()
    if sys.argv[1] == '-s':
        server()
    else:
        usage()


def usage():
    sys.stdout = sys.stderr
    print('Usage: tcpecho -s [port]	(server)')
    sys.exit(2)


def sendData(conn, buffer):
    sent = totalSent = 0
    msgSize = len(buffer)
    print('\r\nSending {} bytes...'.format(msgSize))
    msg = buffer
    while(totalSent < msgSize):
        sent = conn.send(msg)
        if (sent < 0):
            print('Error sending data ({})'.format(sent))
            return
        totalSent += sent
        msg = msg[totalSent:]
        print('Sent {} total: {}'.format(sent, totalSent))
    print('Sent data')


def sendBigData(conn):
    global bigData
    sendData(conn, bigData.encode('ascii'))


def server():
    global conn, s
    asciiData = False
    dataTimeout = False
    strData = ""
    totalRx = rxSize = 0

    if len(sys.argv) > 2:
        port = eval(sys.argv[2])
    else:
        port = ECHO_PORT
    socket.setdefaulttimeout(SOCKET_TIMEOUT_SECS)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print('Listening on port {}'.format(port))
    s.bind(('', port))
    s.listen(1)
    print('TCP echo server ready')
    while True:
        print('Waiting for connection...')
        try:
            conn, addr = s.accept()
        except socket.timeout:
            print('connection timeout')
            continue
        print('Connected by {}'.format(addr))
        totalRx = 0
        dataTimeout = False
        while True:
            print('Waiting for data...')
            try:
                data = conn.recv(BUFSIZE)
            except socket.timeout:
                print('Wait for data timeout')
                dataTimeout = True

            if dataTimeout:
                try:
                    conn.close()
                except:
                    pass
                break

            if not data:
                print('Client closed connection')
                conn.close()
                break
            rxSize = len(data)
            totalRx += rxSize
            print('\r\nRXd {} bytes, Total: {}'.format(rxSize, totalRx))

            try:
                strData = str(data, 'utf-8')
                asciiData = True
            except:
                pass

            if asciiData:
                if 'close' in strData:
                    print('Server closing connection')
                    conn.close()
                    break
                elif 'getBigData' in strData:
                    print('Sending big data...')
                    sendBigData(conn)
                    conn.close()
                    break
                else:
                    print('data:\r\n{}'.format(strData))
            else:
                print('data:\r\n{}'.format(data))
            # echo the data back
            sendData(conn, data)


main()
