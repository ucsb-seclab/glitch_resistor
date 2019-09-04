import os
import pickle
import pprint
import sys
import serial
import logging
from os import listdir
from os.path import isfile, join

logger = logging.getLogger(__name__)

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port='/dev/tty.usbmodem144203',
    baudrate=38400,
    parity=serial.PARITY_ODD,
    stopbits=serial.STOPBITS_TWO,
    bytesize=serial.SEVENBITS
)


def program_board(bin_filename):
    """ Program a bin to the board """
    ser.reset_input_buffer()
    os.system("st-flash --format binary --flash=512k write %s 0x08000000" % bin_filename)


def reset_board():
    """ Reset the board """
    ser.reset_input_buffer()
    os.system("st-flash reset > /dev/null 2>&1")


def get_boot_time():
    """ Return the boot time as an integer """
    while 1:
        line = ser.readline().strip()
        line = line.decode("ascii")
        print(line)
        if "Boot time" in line:
            return int(line.split(":")[1].strip())


if not ser.isOpen():
    print("Failed to open serial port!")
    sys.exit(0)

output_path = "output"
only_files = [f for f in listdir(output_path) if isfile(join(output_path, f))]
only_bins = [f for f in only_files if "bin" in f]

boot_times = {}
for f in only_bins:
    filename = os.path.join(output_path, f)
    program_board(filename)
    boot_times[f] = []
    for x in range(10):
        reset_board()
        boot_time = get_boot_time()
        print(boot_time)
        boot_times[f].append(boot_time)

pprint.pprint(boot_times)

output_filename = os.path.join(output_path, "boot_times.pickle")
pickle.dump(boot_times, open(output_filename, "wb"))
