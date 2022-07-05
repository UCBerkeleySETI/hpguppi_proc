Upchannelizer beamformer library

To compile the standalone code, make sure the main() function in the .cu script is uncommented and use this command:
- `nvcc -o upchannelizer_beamformer_main -arch=sm_86 upchannelizer_beamformer_main.cu upchannelizer_beamformer.cu -lcufft`

To run the executable:
- `./upchannelizer_beamformer_main <output file> <simulated data flag> <simulated coefficients flag> <telescope flag> <mode flag or VLASS specifications depending on telescope flag>`
For help on how to run the executable, type in `-h` or `--help` as the first argument for more information:
- `./upchannelizer_beamformer_main -h`

The post-processing scripts in the post_processing directory are used for analysis and verification.

The plot_upchan_beamformer_output_binary_py3.py is the script currently being used to analyze the output of the standalone code.
Inorder to run this script, a few parameters need to be specified in the script. The following variables will need to be changed depending on the mode that was run:
- `filename` - The output file of the upchannelizer beamformer standalone code
- `telescope_flag` - The telescope i.e. `"MK"` or `"VLA"` for now
- `mode_flag` - And the mode e.g. for MK; `"1k"`, `"4k"` or `"32k"`, and for VLA; `"req"` or `"des"` (required or desired specifications)
- `N_beam` - Number of beams
- `N_time` - Number of time samples remaining after integration
- `N_fine` - Number of fine channels only relevant with the desired specification of the `"VLA"` mode
An example of the command is as follows:
- `python plot_upchan_beamformer_output_binary_py3.py`

Or in VS code, above the `#%%` indicating the beginning of a cell, click on one of the options to run the code.

The plot_cufft_output_binary.py reads a raw binary file with the output of the data that doesn't have an FFT shift and it applies one to the data.

The plot_cufft_output_with_fftshift.py reads a raw binary file with the output of the data that has an FFT shift applied and generates plots assuming an FFT shift was applied.

And the fft_verification.py script reads a raw binary files with the simulated input data that is also going to processed by the cuFFT and performs an FFT with numpy.fft for comparison with the output from the upchannelizer code.
