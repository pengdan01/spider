/*
 Copyright (c) 2012 clowwindy

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

if (process.argv.length < 4) {
    console.error('usage: node local.js server_ip local_port');
    process.exit(1);
}

var SERVER = process.argv[2];
var REMOTE_PORT = 8387;
var PORT = +process.argv[3];
var KEY = 'onebox!';
var timeout = 30000;

var net = require('net');
var encrypt = require('./encrypt.js');
console.log('calculating ciphers');
var tables = encrypt.getTable(KEY);
var encryptTable = tables[0];
var decryptTable = tables[1];

function inetNtoa(buf) {
    return buf[0] + '.' + buf[1] + '.' + buf[2] + '.' + buf[3];
}

function inetAton(ipStr) {
    var parts = ipStr.split('.');
    if (parts.length != 4) {
        return null;
    } else {
        var buf = new Buffer(4);
        for (var i = 0; i < 4; i++)
            buf[i] = +parts[i];
        return buf;
    }
}

var server = net.createServer(function (connection) { //'connection' listener
    console.log('server connected');
    console.log('concurrent connections: ' + server.connections);

    var stage = 0, headerLength = 0, remote = null, cachedPieces = [],
        addrLen = 0, remoteAddr = null, remotePort = null, addrToSend = '';
    connection.on('data', function (data) {
//        encrypt.encrypt(decryptTable, data);
        if (stage == 5) {
            // pipe sockets
            encrypt.encrypt(encryptTable, data);
            if (!remote.write(data)) {
                connection.pause();
            }
            return;
        }
        if (stage == 0) {
            var tempBuf = new Buffer(2);
            tempBuf.write('\x05\x00', 0);
//                encrypt.encrypt(encryptTable, tempBuf);
            connection.write(tempBuf);
            stage = 1;
            return;
        }
        if (stage == 1) { // note this must be if, not else if!
            try {
                // +----+-----+-------+------+----------+----------+
                // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
                // +----+-----+-------+------+----------+----------+
                // | 1  |  1  | X'00' |  1   | Variable |    2     |
                // +----+-----+-------+------+----------+----------+

                // cmd and addrtype
                var cmd = data[1];
                var addrtype = data[3];
                if (cmd != 1) {
                    console.warn('unsupported cmd: ' + cmd);
                    var reply = new Buffer('\x05\x07\x00\x01', 'binary');
//                    encrypt.encrypt(encryptTable, reply);
                    connection.end(reply);
                    return;
                }
                if (addrtype == 3) {
                    addrLen = data[4];
                } else if (addrtype != 1) {
                    console.warn('unsupported addrtype: ' + addrtype);
                    connection.end();
                    return;
                }
                addrToSend = data.slice(3, 4).toString('binary');
                // read address and port
                if (addrtype == 1) {
                    remoteAddr = inetNtoa(data.slice(4, 8));
                    addrToSend += data.slice(4, 10).toString('binary');
                    remotePort = data.readUInt16BE(8);
                    headerLength = 10;
                } else {
                    remoteAddr = data.slice(5, 5 + addrLen).toString('binary');
                    addrToSend += data.slice(4, 5 + addrLen + 2).toString('binary');
                    remotePort = data.readUInt16BE(5 + addrLen);
                    headerLength = 5 + addrLen + 2;
                }
                var buf = new Buffer(10);
                buf.write('\x05\x00\x00\x01', 0, 4, 'binary');
                buf.write('\x00\x00\x00\x00', 4, 4, 'binary');
                buf.writeInt16BE(remotePort, 8);
                connection.write(buf);
                // connect remote server
                remote = net.connect(REMOTE_PORT, SERVER, function () {
                    console.log('connecting ' + remoteAddr);

                    var addrToSendBuf = new Buffer(addrToSend, 'binary');
//                    console.log(addrToSend);
                    encrypt.encrypt(encryptTable, addrToSendBuf);
                    remote.write(addrToSendBuf);
//                    encrypt.encrypt(encryptTable, buf);
                    for (var i = 0; i < cachedPieces.length; i++) {
                        var piece = cachedPieces[i];
                        encrypt.encrypt(encryptTable, piece);
                        remote.write(piece);
                    }
                    cachedPieces = null; // save memory
                    stage = 5;
                });
                remote.on('data', function (data) {
                    encrypt.encrypt(decryptTable, data);
                    if (!connection.write(data)) {
                        remote.pause();
                    }
                });
                remote.on('end', function () {
                    console.log('remote disconnected');
                    connection.end();
                    console.log('concurrent connections: ' + server.connections);
                });
                remote.on('error', function () {
                    if (stage == 4) {
                        console.warn('remote connection refused');
                        var reply = new Buffer('\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00', 'binary');
//                        encrypt.encrypt(encryptTable, reply);
                        connection.end(reply);
                        return;
                    }
                    console.warn('remote error');
                    connection.end();
                    console.log('concurrent connections: ' + server.connections);
                });
                remote.on('drain', function () {
                    connection.resume();
                });
                remote.setTimeout(timeout, function () {
                    connection.end();
                    remote.destroy();
                });
                if (data.length > headerLength) {
                    // make sure no data is lost
                    var buf = new Buffer(data.length - headerLength);
                    data.copy(buf, 0, headerLength);
                    cachedPieces.push(buf);
                    buf = null;
                }
                stage = 4;
            } catch (e) {
                // may encouter index out of range
                console.warn(e);
                connection.destroy();
                if (remote) {
                    remote.destroy();
                }
            }
        } else if (stage == 4) { // note this must be else if, not if!
            // remote server not connected
            // cache received buffers
            // make sure no data is lost
            cachedPieces.push(data);
        }
    });
    connection.on('end', function () {
        console.log('server disconnected');
        if (remote) {
            remote.destroy();
        }
        console.log('concurrent connections: ' + server.connections);
    });
    connection.on('error', function () {
        console.warn('server error');
        if (remote) {
            remote.destroy();
        }
        console.log('concurrent connections: ' + server.connections);
    });
    connection.on('drain', function () {
        // calling resume() when remote not is connected will crash node.js
        if (remote && stage == 5) {
            remote.resume();
        }
    });
    connection.setTimeout(timeout, function () {
        if (remote) {
            remote.destroy();
        }
        connection.destroy();
    })
});
server.listen(PORT, function () {
    console.log('server listening at port ' + PORT);
});
server.on('error', function (e) {
    if (e.code == 'EADDRINUSE') {
        console.warn('Address in use, aborting');
    }
});

