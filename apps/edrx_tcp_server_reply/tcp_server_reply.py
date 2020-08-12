#! /usr/bin/env python3

# TCP server that sends a message back to the client after connection and every
# x seconds after.
#
# Usage: tcp_server_reply -p [port]	-i [interval to send reply (seconds)]

import sys
import signal
import socket
import time

SOCKET_TIMEOUT_SECS = 30
conn = 0
s = 0


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
    if len(sys.argv) < 4:
        usage()
    if sys.argv[1] == '-p' and sys.argv[3] == '-i':
        server()
    else:
        usage()


def usage():
    sys.stdout = sys.stderr
    print(
        'Usage: tcp_server_reply -p [port] -i [interval to send reply (seconds)]')
    sys.exit(2)


def server():
    global conn, s
    asciiData = False
    dataTimeout = False
    strData = ""
    totalRx = rxSize = 0

    if len(sys.argv) > 4:
        port = eval(sys.argv[2])
        interval = eval(sys.argv[4])
    else:
        raise Exception('Invalid number of arguments')

    socket.setdefaulttimeout(SOCKET_TIMEOUT_SECS)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    print('Listening on port {}'.format(port))
    s.bind(('', port))
    s.listen(1)
    print('TCP server ready')
    while True:
        print('Waiting for connection...')
        try:
            conn, addr = s.accept()
        except socket.timeout:
            print('connection timeout')
            continue
        print('Connected by {}'.format(addr))
        conn.send('Connected client {}'.format(addr))
        while True:
            time.sleep(interval)
            print('Server send data...')
            try:
                sent = conn.send("Server data send")
                if (sent < 0):
                    print('Error sending data ({})'.format(sent))
                    try:
                        conn.close()
                    except:
                        pass
                    break
                else:
                    print('server data sent!')
            except:
                print('Error sending data to client')
                try:
                    conn.close()
                except:
                    pass
                break


main()
