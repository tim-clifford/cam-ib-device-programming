# only small brains use other editors
# vim: set ts=4 sw=4 tw=0 noet
# tab gang, pep8 bad
import serial
import re
import numpy as np
import matplotlib.pyplot as plt

def read_temp_dump(f):
    while not re.match(b"=+BEGIN TEMPERATURE DUMP=+",f.readline()):
        pass
    # This is why python needs a do-while, smh
    l = f.readline()
    while not re.match(b"=+END TEMPERATURE DUMP=+",l):
        # don't assume that there isn't other debugging output,
        # but accept any numbers as temperatures
        m = re.match(b"(\d+(\.\d+)?)",l)
        if m: yield float(m.group(0))
        l = f.readline()

if __name__ == "__main__":
    board = serial.Serial("/dev/ttyACM0", 9600)
    n = 1                       # i miss c style for loops :(
    while True:
        data = np.fromiter(read_temp_dump(board),np.float)
        plt.figure()
        plt.plot(np.arange(-len(data)+1,1,1),data)
        plt.xlabel("Time (s)")
        plt.ylabel("Temp (C)")
        # Just keep going while there are more temperature dumps
        plt.savefig(f"temp_dump_{n}.pdf")
        n += 1                  # i miss ++ :(
