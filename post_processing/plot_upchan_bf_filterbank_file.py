# This script reads a single filterbank file specified in the variable 'filename'
# To run it accordingly, change the variables telescope_flag, mode_flag, and N_time which depends on the number of RAW files as well
# Works with python 3
#%%
import matplotlib.pyplot as plt
from array import array
import numpy as np
import sys

# Open binary file containing beamformer output
#filename = "/datag/users/mruzinda/o/guppi_raw_bfr5_test_JBLAH-BLAH.B01.SB00.B01.fil"
#filename = "/datag/users/mruzinda/o/synth_multi_ant_df-sig-multiBEAM_0.SB08.B00.fil"
filename = "/datag/users/mruzinda/o/synth_multi_ant_df-sig-multi3C84.SB08.B03.fil"

# Read file contents: np.fromfile(filename, dtype=float, count=- 1, sep='', offset=0)
with open(filename, 'rb') as f:
    #f.seek(393)
    #f.seek(377)
    f.seek(375)
    contents_float = np.fromfile(f, dtype=np.float32)

print(len(contents_float))
print(contents_float[0])

telescope_flag = "VLA" #"MK" # Observatory/radio telescope in use
mode_flag = "req" #"4k" # Mode of operation
num_raw_files = 3 # Number of RAW files
N_fine = 262144 # 131072 #5120000
N_subband = 16
subband_idx = 8

# Array dimensions
# MeerKAT specs
if telescope_flag == "MK":
    N_beam = 1 # One in each filterbank file for now
    N_time = 1*num_raw_files # Number of time samples in filterbank file
    # 1k mode
    if mode_flag == "1k":
        N_coarse = 1
        N_fine = (4096*1024)/8 # N_coarse*2^19
    # 4k mode
    if mode_flag == "4k":
        N_coarse = 4 # 4
        N_fine = (1024*1024)/8 # N_coarse*2^17 
    # 32k mode
    if mode_flag == "32k":
        N_coarse = 32
        N_fine = (128*1024)/8 # N_coarse*2^14
    

# VLASS specs
if telescope_flag == "VLA":
    N_coarse = 4
    # Required
    if mode_flag == "req":
        N_win = 8 # The number of FFT windows
        N_time = N_win*num_raw_files #40 # STI windows
        N_fine = 262144 #128000
        N_beam = 1 # Including incoherent beam # 64
        N_subband = 16
        N_total_coarse = N_subband*N_coarse
        subband_shift = N_subband - subband_idx
    # Desired
    if mode_flag == "des":
        N_beam = 1 # Including incoherent beam # 64
        # Number of points of the FFT
        if N_fine == 5120000:
            N_time = 2*num_raw_files # STI windows
        if N_fine == 1253376:
            N_time = 4*num_raw_files # STI windows
        if N_fine == 128000:
            N_time = 80*num_raw_files # STI windows
        if N_fine == 1024:
            N_time = 10000*num_raw_files # STI windows

incoherent_beam_idx = N_beam-1
N_elements = int(N_time*N_coarse*N_fine*N_beam)
print('N_time = '+ str(N_time))
print('N_coarse = '+ str(N_coarse))
print('N_fine = '+ str(N_fine))
print('N_beam = '+ str(N_beam))
print('N_elements = '+ str(N_elements))

obsfreq = 3031.5 # MHz
coarse_chan_bw = 1 #MHz
fine_chan_bw = coarse_chan_bw/N_fine
last_fine_chan_raw = obsfreq + (fine_chan_bw/2) + fine_chan_bw*((((N_subband/2)*N_coarse*N_fine)/2)-1)
first_fine_chan_sb = last_fine_chan_raw - fine_chan_bw*(((subband_shift*N_coarse*N_fine)/2)-1)
last_fine_chan_sb = last_fine_chan_raw - fine_chan_bw*((((subband_shift-1)*N_coarse*N_fine)/2)-1) - fine_chan_bw

#define pow_bf_idx(f, c, s, b, Nf, Nc, Ns)
# Reshape array to 3D -> Fine channel X Coarse channel X Time samples X Beams
#contents_array = contents_float[0:(N_time*N_coarse*N_fine*N_beam)].reshape(N_beam,N_time,N_coarse*N_fine)
contents_array = np.reshape(contents_float[0:(N_elements)], [N_beam,N_time,int(N_coarse*N_fine)])

beam_idx = 0 # beam index to plot
time_idx = 0 # time sample index to plot

if N_time > 1:
    # Plot intensity map of frequency vs. time
    # "interpolation ='none'" removes interpolation which was there by default. 
    # I'm only removing it for the sake of accurate analysis and diagnosis.
    #plt.imshow(contents_array[0:N_time,0:N_fine,beam_idx], extent=[1, N_fine, 1, N_time], aspect='auto', interpolation='bicubic')
    #plt.imshow(10*np.log(contents_array[beam_idx,0:N_time,0:int(N_coarse*N_fine)]), extent=[0, int(N_coarse*N_fine), 0, N_time], aspect='auto', interpolation='none')
    plt.imshow(10*np.log(contents_array[beam_idx,0:N_time,0:int(N_coarse*N_fine)]), extent=[first_fine_chan_sb, last_fine_chan_sb, 0, N_time], aspect='auto', interpolation='none')
    plt.title('Waterfall (Frequency vs. time)')
    plt.ylabel('Time samples')
    plt.xlabel('Frequency (Hz))')
    plt.show()

#print(contents_array[0,0,(N_fine-10):(N_fine-1)])
#print(contents_array[0:N_beam,0,5])

# Plot of power spectrum
plt.plot(10*np.log(contents_array[beam_idx,time_idx,0:int(N_coarse*N_fine)]))
plt.title('Power spectrum')
plt.xlabel('Frequency bins')
plt.ylabel('Power (arb.)')
plt.show()
#%%