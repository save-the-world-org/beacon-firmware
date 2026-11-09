#!/usr/bin/env python3
"""Talk to a serial port with only the standard library.

serialtalk.py PORT [--baud N] [--listen SECONDS] [--send TEXT ...]

Listens first, then sends each --send string (\\r, \\n and \\xNN escapes
understood) and listens again after each one. Prints what arrives, with
non-printable bytes escaped, and a hex dump when --hex is given.
"""
import argparse
import os
import select
import sys
import termios
import time

BAUDS = {
    9600: termios.B9600, 19200: termios.B19200, 38400: termios.B38400,
    57600: termios.B57600, 115200: termios.B115200, 230400: termios.B230400,
}


def open_port(path, baud):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)
    iflag, oflag, cflag, lflag, ispeed, ospeed, cc = attrs
    iflag = 0
    oflag = 0
    lflag = 0
    cflag = termios.CS8 | termios.CREAD | termios.CLOCAL
    cc[termios.VMIN] = 0
    cc[termios.VTIME] = 0
    speed = BAUDS[baud]
    termios.tcsetattr(fd, termios.TCSANOW, [iflag, oflag, cflag, lflag, speed, speed, cc])
    termios.tcflush(fd, termios.TCIOFLUSH)
    return fd


def listen(fd, seconds):
    end = time.monotonic() + seconds
    buf = bytearray()
    while True:
        left = end - time.monotonic()
        if left <= 0:
            break
        r, _, _ = select.select([fd], [], [], min(left, 0.2))
        if r:
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                continue
            buf.extend(chunk)
    return bytes(buf)


def show(label, data, hexdump):
    print(f"--- {label}: {len(data)} bytes")
    if not data:
        return
    text = data.decode("utf-8", errors="backslashreplace")
    out = []
    for ch in text:
        if ch in "\n\t" or (" " <= ch <= "~") or ord(ch) > 127:
            out.append(ch)
        elif ch == "\r":
            continue
        else:
            out.append(f"\\x{ord(ch):02x}")
    print("".join(out))
    if hexdump:
        for i in range(0, len(data), 16):
            row = data[i:i + 16]
            print(f"{i:04x}  {row.hex(' '):<47}  {''.join(chr(b) if 32 <= b < 127 else '.' for b in row)}")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("port")
    p.add_argument("--baud", type=int, default=115200, choices=sorted(BAUDS))
    p.add_argument("--listen", type=float, default=3.0)
    p.add_argument("--after", type=float, default=2.0, help="seconds to listen after each send")
    p.add_argument("--send", action="append", default=[])
    p.add_argument("--gap", type=float, default=0.0, help="seconds between the characters of a send")
    p.add_argument("--hex", action="store_true")
    p.add_argument("--run", help="shell command to run once the port is open (e.g. a reset)")
    a = p.parse_args()

    fd = open_port(a.port, a.baud)
    try:
        if a.run:
            import subprocess
            r = subprocess.run(a.run, shell=True, capture_output=True, text=True)
            print(f"--- ran {a.run!r}: exit {r.returncode} {r.stderr.strip()[-300:]}")
        show(f"listened {a.listen}s at {a.baud}", listen(fd, a.listen), a.hex)
        for s in a.send:
            raw = s.encode().decode("unicode_escape").encode("latin-1")
            if a.gap > 0:
                for b in raw:
                    os.write(fd, bytes([b]))
                    time.sleep(a.gap)
            else:
                os.write(fd, raw)
            show(f"after sending {s!r}", listen(fd, a.after), a.hex)
    finally:
        os.close(fd)


if __name__ == "__main__":
    sys.exit(main())
