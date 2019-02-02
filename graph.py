import re
import numpy as np
import matplotlib.pyplot as plt

t = []
y1 = []
 
data = open('between')
time = 0
for eachLine in data:
    readTmp = data.readline()
    tmp = [float(tmpLine)]
    time += 1
    t.append(time)
    y1.append(tmp)

data.close()
 
plt.plot(t, [i for i in y1])
plt.grid(True)
plt.xlabel('time')
plt.legend(('between'))
plt.axis([0, time, 0, 0.000999])
plt.show()
