#! /usr/bin/env python3

# Client and server for udp (datagram) echo.
#
# Usage: udpecho -s [port]            (to start a server)
# or:    udpecho -c host [port] <file (client)

import sys
import socket

ECHO_PORT = 50000 + 7
BUFSIZE = 1024


def main():
    if len(sys.argv) < 2:
        usage()
    if sys.argv[1] == '-s':
        server()
    elif sys.argv[1] == '-c':
        client()
    else:
        usage()


def usage():
    sys.stdout = sys.stderr
    print('Usage: udpecho -s [port]            (server)')
    print('or:    udpecho -c host [port] <file (client)')
    sys.exit(2)


def server():
    if len(sys.argv) > 2:
        port = eval(sys.argv[2])
    else:
        port = ECHO_PORT
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('', port))
    print('udp echo server ready')
    while 1:
        data, addr = s.recvfrom(BUFSIZE)
        print('server received [{}] from {}'.format(data, addr))
        s.sendto(data, addr)


def client():
    if len(sys.argv) < 3:
        usage()
    host = sys.argv[2]
    if len(sys.argv) > 3:
        port = eval(sys.argv[3])
    else:
        port = ECHO_PORT
    addr = host, port
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('', 0))
    print('udp echo client ready, reading stdin')
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        print('addr = {}'.format(addr))
        s.sendto(bytes(line, 'ascii'), addr)
        data, fromaddr = s.recvfrom(BUFSIZE)
        print('client received {} from {}'.format(data, fromaddr))


main()
