import pylab as plt
from blimpy import Waterfall
import numpy as np
import sys

file_path = sys.argv[1]
base_path = file_path.split('fil')[0]
f_start = 3032.2496 # Starting frequency in MHz
f_stop = 3032.2504 # Stopping frequency in MHz
ics = Waterfall(file_path, f_start, f_stop)

less_than_zero = np.where(ics.data == 0)
print("Number of values equal to zero = ", str(np.size(less_than_zero)))

print("After waterfall function, and file_path is: ", file_path)
print("base_path is: ", base_path)

fig = plt.figure(figsize=[10,10])
plt.subplots_adjust(left=None, bottom=None, right=None, top=None, wspace=0.2, hspace=0.3)

ax = plt.subplot2grid((2,1),(0,0))
ics.plot_waterfall(cb=False)
ax2 = plt.subplot2grid((2,1),(1,0))
ics.plot_spectrum(logged=True)
plt.show()
#plt.savefig(base_path+'png')
