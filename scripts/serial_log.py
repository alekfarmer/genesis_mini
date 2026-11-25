"""Record a board's serial output for a fixed time, without a terminal.

Usage (through the Makefile only): make log PORT=/dev/cu.usbmodemXXXX SECONDS=10

Opens the port with DTR and RTS held off so opening it does not reset the
ESP32-S3, prints every line received for SECONDS, then exits. Read-only: it
never writes to the board.
"""

import sys
import time

import serial

BAUD = 115200
MAX_SECONDS = 600


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: serial_log.py PORT SECONDS", file=sys.stderr)
        return 2
    port, seconds_arg = sys.argv[1], sys.argv[2]
    try:
        seconds = float(seconds_arg)
    except ValueError:
        print(f"SECONDS must be a number, got {seconds_arg!r}", file=sys.stderr)
        return 2
    if not 0 < seconds <= MAX_SECONDS:
        print(f"SECONDS must be between 0 and {MAX_SECONDS}", file=sys.stderr)
        return 2

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = BAUD
    ser.timeout = 0.2
    ser.dtr = False  # set before open(): toggling these lines can reset the chip
    ser.rts = False
    try:
        ser.open()
    except serial.SerialException as exc:
        print(f"cannot open {port}: {exc} (is a monitor still running?)", file=sys.stderr)
        return 1

    print(f"--- logging {port} for {seconds:g} s ---", flush=True)
    deadline = time.monotonic() + seconds
    pending = b""
    try:
        while time.monotonic() < deadline:
            try:
                chunk = ser.read(256)
            except serial.SerialException as exc:
                # Native USB drops when the board resets; report and keep trying.
                print(f"--- port lost ({exc}), reconnecting ---", flush=True)
                ser.close()
                while time.monotonic() < deadline:
                    time.sleep(0.5)
                    try:
                        ser.open()
                        print("--- reconnected ---", flush=True)
                        break
                    except serial.SerialException:
                        continue
                continue
            if not chunk:
                continue
            pending += chunk
            *lines, pending = pending.split(b"\n")
            for line in lines:
                print(line.decode("utf-8", errors="replace").rstrip("\r"), flush=True)
    finally:
        if pending:
            print(pending.decode("utf-8", errors="replace"), flush=True)
        ser.close()
    print("--- done ---", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
