"""Test sender: sweeps the light gun crosshair in a circle and fires every second."""
import math
import socket
import sys
import time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 27015
seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 60
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
t0 = time.perf_counter()
while (t := time.perf_counter() - t0) < seconds:
    x = 0.5 + 0.3 * math.cos(t * 1.5)
    y = 0.5 + 0.3 * math.sin(t * 1.5)
    fire = 1 if (t % 1.0) < 0.1 else 0
    sock.sendto(f"LG 0 {round(x * 10000)} {round(y * 10000)} {fire}".encode(), ("127.0.0.1", port))
    time.sleep(1 / 60)
