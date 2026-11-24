#!/usr/bin/env python3
# Serial logger used by esp_bridge.sh: writes timestamped serial output to <dir>/monitor.log,
# sends to the port the content of <dir>/send when it appears.
import os, sys, time, serial

port, d, mode = sys.argv[1], sys.argv[2], sys.argv[3]
logpath = os.path.join(d, 'monitor.log')
sendpath = os.path.join(d, 'send')

ser = serial.Serial()
ser.port = port
ser.baudrate = 115200
ser.timeout = 0.05
ser.dtr = False   # do not touch EN/GPIO0 when opening the port
ser.rts = False
ser.open()
if mode == 'reset':
    # classic ESP32 auto-reset circuit: EN low pulse, GPIO0 high
    ser.dtr = False
    ser.rts = True
    time.sleep(0.1)
    ser.rts = False
    open(logpath, 'w').close()

t0 = time.time()
buf = b''
with open(logpath, 'a', buffering=1) as log:
    while True:
        data = ser.read(1024)
        if data:
            buf += data
            while b'\n' in buf:
                line, buf = buf.split(b'\n', 1)
                log.write('%8.3f %s\n' % (time.time() - t0, line.decode('utf-8', 'replace').rstrip('\r')))
        if os.path.exists(sendpath):
            try:
                txt = open(sendpath).read().rstrip('\n')
                os.remove(sendpath)
                ser.write(txt.encode() + b'\r')
                log.write('%8.3f >>> SENT: %s\n' % (time.time() - t0, txt))
            except OSError:
                pass
