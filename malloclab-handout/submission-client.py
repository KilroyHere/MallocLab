import socket
import sys
from os import path

HOST, PORT = "lnxsrv06.seas.ucla.edu", 18213
username = sys.argv[1]

# Create a socket (SOCK_STREAM means a TCP socket)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    # Connect to server and send data
    sock.connect((HOST, PORT))
    filename = username + '-mm.c'
    filename_len = f'{len(filename):08x}'

    sock.sendall(bytes(filename_len, 'ascii'))
    sock.sendall(bytes(filename, 'ascii'))

    filesize = path.getsize("mm.c")
    filesize_hex = f'{filesize:08x}'
    sock.sendall(bytes(filesize_hex, 'ascii'))

    with open("mm.c", 'rb') as f:
        while True:
            data = f.read(4096)
            if not data:
                break;
            sock.sendall(data)

    print(f"Sent mm.c as {filename}: {filesize} bytes sent")

