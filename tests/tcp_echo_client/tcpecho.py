#! /usr/bin/env python3

# Server for TCP echo.
#
# Usage: tcpecho -s [port]	(to start a server)

import sys
import signal
import socket

ECHO_PORT = 50000 + 7
BUFSIZE = 2048
conn = 0
s = 0


def sig_handling(signum, frame):
    global conn, s
    try:
        conn.shutdown(1)
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


def server():
    global conn, s
    if len(sys.argv) > 2:
        port = eval(sys.argv[2])
    else:
        port = ECHO_PORT
    socket.setdefaulttimeout(30)
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
        while True:
            print('Waiting for data...')
            try:
                data = conn.recv(BUFSIZE)
            except socket.timeout:
                print('Wait for data timeout')
                break
            if data == 'close':
                print('Server closing connection')
                conn.shutdown(1)
                conn.close()
                break
            elif data:
                print('server received: {}'.format(data))
                conn.send(data)
            elif not data:
                print('Client closed connection')
                conn.shutdown(1)
                conn.close()
                break


main()
