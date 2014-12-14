#!/usr/bin/env python

import os
import sys
import termios
import fcntl
import socket
import select


IFLAG = 0
OFLAG = 1
CFLAG = 2
LFLAG = 3
ISPEED = 4
OSPEED = 5
CC = 6

# from https://github.com/wiseman/arduino-serial/blob/master/arduinoserial.py
def open_serial(devname='/dev/ttyUSB0', baud=termios.B115200):
    fd = os.open(devname, os.O_RDWR | os.O_NOCTTY | os.O_NDELAY)

    attrs = termios.tcgetattr(fd)

    attrs[ISPEED] = baud
    attrs[OSPEED] = baud
    
    # 8N1
    attrs[CFLAG] &= ~termios.PARENB
    attrs[CFLAG] &= ~termios.CSTOPB
    attrs[CFLAG] &= ~termios.CSIZE
    attrs[CFLAG] |= termios.CS8
    
    # No flow control
    attrs[CFLAG] &= ~termios.CRTSCTS
    
    # Turn on READ & ignore control lines.
    attrs[CFLAG] |= termios.CREAD | termios.CLOCAL
    
    # Turn off software flow control.
    attrs[IFLAG] &= ~(termios.IXON | termios.IXOFF | termios.IXANY)
    
    # Make raw.
    attrs[LFLAG] &= ~(termios.ICANON | termios.ECHO | termios.ECHOE | termios.ISIG)
    attrs[OFLAG] &= ~termios.OPOST
    
    # It's complicated--See
    # http://unixwiz.net/techtips/termios-vmin-vtime.html
    attrs[CC][termios.VMIN] = 0;
    attrs[CC][termios.VTIME] = 20;
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

    return os.fdopen(fd, 'w+b')

if __name__ == '__main__':
    
    serial_name = sys.argv[1]
    socket_name = './' + os.path.basename(serial_name)
    
    serial_file = open_serial(serial_name)

    if os.path.exists(socket_name):
        os.unlink(socket_name)

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(socket_name)
    
    while True:
        print 'Listening for a connection...'
        sock.listen(1)
        connection, client_address = sock.accept() 

        print 'Connected!', client_address
        
        read_files = [connection, serial_file]
        excpt_files = read_files

        while connection:
            rlist, _, xlist = select.select(read_files, [], excpt_files)

            for f in xlist:
                if f == connection:
                    print "exception on connection"
                    exit(1)
                elif f == serial_file:
                    print "exception on serial file"
                    exit(1)
                else:
                    assert(False and "exception from unknown file.")

            for f in rlist:
                if f == connection:
                    data = connection.recv(4096)
                    if len(data) == 0:
                        print 'Client disconnected.'
                        connection = None
                    else:
                        #print 'Client sent:', data
                        serial_file.write(data)
                elif f == serial_file:
                    data = serial_file.read()
                    if len(data) == 0:
                        print 'Serial port is closed?!?!'
                        exit(1)
                    else:
                        #print 'Serial sent:', data
                        connection.send(data)
                else:
                    assert(False and "an unknown file is readable")
 
