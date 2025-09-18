#!/usr/bin/env python3
import serial
import time
import sys

devices = ["/dev/tty.usbmodem101", "/dev/tty.usbmodemm59111127381"]

for device in devices:
    print(f"Trying {device}...")
    try:
        ser = serial.Serial(device, 115200, timeout=2)
        print(f"Successfully opened {device}")

        print("Listening for 10 seconds...")
        start_time = time.time()
        while time.time() - start_time < 10:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"Received: {line}")
            time.sleep(0.1)

        ser.close()
        print(f"Closed {device}")
        break

    except Exception as e:
        print(f"Failed to open {device}: {e}")