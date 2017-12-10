#!/usr/bin/env python

# Copyright (c) 2012 clowwindy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import sys

if len(sys.argv) < 3:
  print 'usage: local.py server_ip local_port'
  sys.exit(1)

SERVER = sys.argv[1]
REMOTE_PORT = 8388
PORT = int(sys.argv[2])
KEY = "onebox!"

import socket
import select
import SocketServer
import struct
import string
import hashlib
import time
import sys

def get_table(key):
    m = hashlib.md5()
    m.update(key)
    s = m.digest()
    (a, b) = struct.unpack('<QQ', s)
    table = [c for c in string.maketrans('', '')]
    for i in xrange(1, 1024):
        table.sort(lambda x, y: int(a % (ord(x) + i) - a % (ord(y) + i)))
    return table


class ThreadingTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass


class Socks5Server(SocketServer.StreamRequestHandler):
    def handle_tcp(self, sock, remote):
        try:
            fdset = [sock, remote]
            while True:
                r, w, e = select.select(fdset, [], [])
                if sock in r:
                    if remote.send(self.encrypt(sock.recv(4096))) <= 0:
                        break
                if remote in r:
                    if sock.send(self.decrypt(remote.recv(4096))) <= 0:
                        break
        finally:
            remote.close()

    def encrypt(self, data):
      return data.translate(encrypt_table)

    def decrypt(self, data):
      return data.translate(decrypt_table)

    def send_encrpyt(self, sock, data):
      sock.send(self.encrypt(data))

    def handle(self):
        try:
            print 'socks connection from ', self.client_address
            sock = self.connection
            sock.recv(262)
            sock.send("\x05\x00")
            data = self.rfile.read(4)
            mode = ord(data[1])
            if mode != 1:
              print 'mode != 1'
              return
            addrtype = ord(data[3])
            addr_to_send = data[3]
            if addrtype == 1:
                addr_ip = self.rfile.read(4)
                addr = socket.inet_ntoa(addr_ip)
                addr_to_send += addr_ip
            elif addrtype == 3:
                addr_len = sock.recv(1)
                addr = self.rfile.read(ord(addr_len))
                addr_to_send += addr_len + addr
            else:
              print 'not support'
                # not support
              return
            addr_port = self.rfile.read(2)
            addr_to_send += addr_port
            port = struct.unpack('>H', addr_port)
            reply = "\x05\x00\x00\x01"
            reply += socket.inet_aton('0.0.0.0') + struct.pack(">H", 2222)
            sock.send(reply)
            try:
                if mode == 1:
                    remote = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    remote.connect((SERVER, REMOTE_PORT))
                    self.send_encrpyt(remote, addr_to_send)
                    print 'Tcp connect to', addr, port[0]
                else:
                    reply = "\x05\x07\x00\x01" # Command not supported
                    print 'command not supported'
                    return
            except socket.error as e:
                # Connection refused
                reply = '\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00'
                print 'socket error ' + str(e)
                return
            if reply[1] == '\x00':
                if mode == 1:
                    self.handle_tcp(sock, remote)
        except socket.error as e:
            print 'socket error ' + str(e)


def main():
    if '-6' in sys.argv[1:]:
        ThreadingTCPServer.address_family = socket.AF_INET6
    server = ThreadingTCPServer(('0.0.0.0', PORT), Socks5Server)
    server.allow_reuse_address = True
    print "starting server at port %d ..." % PORT
    server.serve_forever()

if __name__ == '__main__':
    encrypt_table = ''.join(get_table(KEY))
    decrypt_table = string.maketrans(encrypt_table, string.maketrans('', ''))
    main()
