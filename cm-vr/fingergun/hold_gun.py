"""
Test sender: holds the light gun at one position.

    hold_gun.py x y seconds [port] [fire_every_s] [player] [bits]

Every fire_every_s seconds the bits (default 1 = trigger; 2 = reload, 4 = Start) are sent for
0.15 s. fire_every_s 0 (or left out) never fires, unless bits are given: then they are sent
once, at the start. player 0 = P1, 1 = P2. E.g. P2's Start once at the title screen:
hold_gun.py 0.7 0.5 3 27015 0 1 4
"""
import socket
import sys
import time

x, y, seconds = float(sys.argv[1]), float(sys.argv[2]), float(sys.argv[3])
port = int(sys.argv[4]) if len(sys.argv) > 4 else 27015
fire_every = float(sys.argv[5]) if len(sys.argv) > 5 else 0
player = int(sys.argv[6]) if len(sys.argv) > 6 else 0
explicit_bits = len(sys.argv) > 7
bits = int(sys.argv[7]) if explicit_bits else 1
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
start = time.time()
while (t := time.time() - start) < seconds:
    on = (t % fire_every) < 0.15 if fire_every else (explicit_bits and t < 0.15)
    sock.sendto(f"LG {player} {round(x * 10000)} {round(y * 10000)} {bits if on else 0}".encode(), ("127.0.0.1", port))
    time.sleep(0.004)
