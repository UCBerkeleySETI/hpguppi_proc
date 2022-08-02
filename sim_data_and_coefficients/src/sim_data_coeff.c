#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
//#include <iostream>
#include <string.h>
#include <math.h>
#include "sim_data_coeff.h"

// Generate simulated data
signed char *simulate_data_ubf(int n_sim_ant, int nants, int n_pol, int n_chan, int nt, int n_win, int sim_flag, int telescope_flag, float rect_zero_samps, float freq_band_shift)
{
	int n_input = 0;
	int n_ant_config = 0;
	if (telescope_flag == 0)
	{
		n_input = N_INPUT;
		n_ant_config = N_ANT;
	}
	else if (telescope_flag == 1)
	{
		n_input = VLASS_N_INPUT;
		n_ant_config = N_ANT / 2;
	}

	signed char *data_sim;
	data_sim = (signed char *)calloc(n_input, sizeof(signed char));

	/*
	'sim_flag' is a flag that indicates the kind of data that is simulated.
	sim_flag = 0 -> Ones
	sim_flag = 1 -> Ones placed in a particular bin (bin 3 for now)
	sim_flag = 2 -> Ones placed in a particular bin at a particular antenna (bin 3 and antenna 3 for now)
	sim_flag = 3 -> Rect placed in a particular bin at a particular antenna (bin 3 and antenna 3 for now)
	sim flag = 4 -> Simulated cosine wave
	sim flag = 5 -> Simulated complex exponential i.e. exp(j*2*pi*f0*t)
	sim flag = 6 -> Simulated drifting signal or complex exponential i.e. exp(j*2*pi*f0*t)
	*/
	// int sim_flag = 5;
	if (sim_flag == 0)
	{
		for (int i = 0; i < (n_input / 2); i++)
		{
			if (i < ((n_sim_ant * n_input) / (2 * n_ant_config)))
			{
				data_sim[2 * i] = 1;
			}
			else
			{
				data_sim[2 * i] = 0;
			}
		}
	}
	if (sim_flag == 1)
	{
		for (int p = 0; p < n_pol; p++)
		{
			for (int t = 0; t < nt; t++)
			{
				for (int a = 0; a < nants; a++)
				{
					if (a < n_sim_ant)
					{
						data_sim[2 * data_in_idx(p, t, 0, a, 2, n_pol, nt, n_win, nants)] = 1;
					}
				}
			}
		}
	}
	if (sim_flag == 2)
	{
		for (int p = 0; p < n_pol; p++)
		{
			for (int t = 0; t < nt; t++)
			{
				data_sim[2 * data_in_idx(p, t, 0, 2, 2, n_pol, nt, n_win, nants)] = 1;
			}
		}
	}
	if (sim_flag == 3)
	{
		// data_in_idx(p, t, w, c, a, Np, Nt, Nw, Nc)
		for (int p = 0; p < n_pol; p++)
		{
			for (int t = (1024 * 10); t < (nt - (1024 * 10)); t++)
			{
				// data_sim[2 * data_in_idx(p, 0, 2, t, 0, n_pol, n_chan, nants, nt)] = 1;
				// data_sim[2 * data_in_idx(p, 1, 2, t, 0, n_pol, n_chan, nants, nt)] = 1;
				data_sim[2 * data_in_idx(p, t, 0, 2, 2, n_pol, nt, n_win, nants)] = 1;
				// data_sim[2 * data_in_idx(p, 3, 2, t, 0, n_pol, n_chan, nants, nt)] = 1;

				data_sim[2 * data_in_idx(p, t, 0, 2, 2, n_pol, nt, n_win, nants) + 1] = 1;
			}
		}
	}
	if (sim_flag == 4)
	{
		float freq = 1e3; // Resonant frequency

		float tmp_max = 1.0;
		float tmp_min = -1.0;

		for (int t = 0; t < nt; t++)
		{
			for (int f = 0; f < n_chan; f++)
			{
				for (int a = 0; a < nants; a++)
				{
					if (a < n_sim_ant)
					{
						// Requantize from doubles/floats to signed chars with a range from -128 to 127
						// X polarization
						data_sim[2 * data_in_idx(0, t, 0, a, f, n_pol, nt, n_win, nants)] = (signed char)((((cos(2 * PI * freq * t * 0.000001) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
						// data_sim[2 * data_in_idx(0, f, a, t, 0, n_pol, n_chan, nants, nt) + 1] = 0;
						//  Y polarization
						data_sim[2 * data_in_idx(1, t, 0, a, f, n_pol, nt, n_win, nants)] = (signed char)((((2 * cos(2 * PI * freq * t * 0.000001) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
						// data_sim[2 * data_in_idx(1, f, a, t, 0, n_pol, n_chan, nants, nt) + 1] = 0;
					}
				}
			}
		}
	}
	if (sim_flag == 5)
	{
		float freq = 1e3; // Resonant frequency
		float tmp_max = 1.0;
		float tmp_min = -1.0;

		for (int w = 0; w < n_win; w++)
		{
			for (int t = 0; t < nt; t++)
			{
				for (int f = 0; f < n_chan; f++)
				{
					for (int a = 0; a < nants; a++)
					{
						if (a < n_sim_ant)
						{
							// Requantize from doubles/floats to signed chars with a range from -128 to 127
							// X polarization
							data_sim[2 * data_in_idx(0, t, w, a, f, n_pol, nt, n_win, nants)] = (signed char)((((cos(2 * PI * freq * t * 0.000001) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							data_sim[2 * data_in_idx(0, t, w, a, f, n_pol, nt, n_win, nants) + 1] = (signed char)((((sin(2 * PI * freq * t * 0.000001) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							// data_sim[2 * data_in_idx(0, f, a, t, 0, n_pol, n_chan, nants, nt) + 1] = 0;
							//  Y polarization
							data_sim[2 * data_in_idx(1, t, w, a, f, n_pol, nt, n_win, nants)] = (signed char)((((2 * cos(2 * PI * freq * t * 0.000001) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							data_sim[2 * data_in_idx(1, t, w, a, f, n_pol, nt, n_win, nants) + 1] = (signed char)((((sin(2 * PI * freq * t * 0.000001) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							// data_sim[2 * data_in_idx(1, f, a, t, 0, n_pol, n_chan, nants, nt) + 1] = 0;
						}
					}
				}
			}
		}
	}
	if (sim_flag == 6)
	{
		printf("Sim flag 6\n");
		float freq = 1e9; // Resonant frequency
		//float shift_freq = 0; // Shifting frequency depending on time sample
		float sig_shift = 0;
		float tmp_max = 1.0;
		float tmp_min = -1.0;
		//int freq_band_shift = 10000;

		for (int w = 0; w < n_win; w++)
		{
			for (int t = rect_zero_samps; t < (nt - rect_zero_samps); t++)
			//for (int t = 0; t < nt; t++)
			{
				for (int f = 0; f < n_chan; f++)
				{
					for (int a = 0; a < nants; a++)
					{
						if (a < n_sim_ant)
						{
							//shift_freq = freq + ((t) - (nt/2));
							sig_shift = freq_band_shift*w;
							// Requantize from doubles/floats to signed chars with a range from -128 to 127
							// X polarization
							data_sim[2 * data_in_idx(0, t, w, a, f, n_pol, nt, n_win, nants)] = (signed char)((((cos(2 * PI * (freq + sig_shift) * t) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							data_sim[2 * data_in_idx(0, t, w, a, f, n_pol, nt, n_win, nants) + 1] = (signed char)((((sin(2 * PI * (freq + sig_shift) * t) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							// data_sim[2 * data_in_idx(0, f, a, t, 0, n_pol, n_chan, nants, nt) + 1] = 0;
							//  Y polarization
							data_sim[2 * data_in_idx(1, t, w, a, f, n_pol, nt, n_win, nants)] = (signed char)((((2 * cos(2 * PI * (freq + sig_shift) * t) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							data_sim[2 * data_in_idx(1, t, w, a, f, n_pol, nt, n_win, nants) + 1] = (signed char)((((sin(2 * PI * (freq + sig_shift) * t) - tmp_min) / (tmp_max - tmp_min)) - 0.5) * 256);
							// data_sim[2 * data_in_idx(1, f, a, t, 0, n_pol, n_chan, nants, nt) + 1] = 0;
						}
					}
				}
			}
		}
	}
	return data_sim;
}

// Generate simulated weights or coefficients
float *simulate_coefficients_ubf(int n_sim_ant, int nants, int n_pol, int n_beam, int n_chan, int sim_flag, int telescope_flag)
{
	int n_coeff = 0;
	int n_ant_config = 0;
	if (telescope_flag == 0)
	{
		n_coeff = N_COEFF;
		n_ant_config = N_ANT;
	}
	else if (telescope_flag == 1)
	{
		n_coeff = VLASS_N_COEFF;
		n_ant_config = N_ANT / 2;
	}

	float *coeff_sim;
	coeff_sim = (float *)calloc(n_coeff, sizeof(float));
	/*
	'sim_flag' is a flag that indicates the kind of data that is simulated.
	sim_flag = 0 -> Ones
	sim_flag = 1 -> Scale each beam by incrementing value i.e. beam 1 = 1, beam 2 = 2, ..., beam 64 = 64
	sim_flag = 2 -> Scale each beam by incrementing value in a particular bin (bin 3 and 6 for now). Match simulated data sim_flag = 2
	sim flag = 3 -> Simulated beams from 58 to 122 degrees. Assuming a ULA.
	sim_flag = 4 -> One value at one polarization, one element, one beam, and one frequency bin
	*/
	if (sim_flag == 0)
	{
		for (int i = 0; i < (n_pol * n_ant_config * n_beam * n_chan); i++)
		{
			coeff_sim[2 * i] = 1;
			// coeff_sim[2*i + 1] = 1;
		}
	}
	if (sim_flag == 1)
	{
		int tmp = 0;

		for (int p = 0; p < n_pol; p++)
		{
			for (int f = 0; f < n_chan; f++)
			{
				for (int a = 0; a < nants; a++)
				{
					for (int b = 0; b < n_beam; b++)
					{
						if (tmp >= n_beam)
						{
							tmp = 0;
						}
						coeff_sim[2 * coeff_idx(a, p, b, f, nants, n_pol, n_beam)] = tmp * 0.01;
						tmp = (tmp + 1) % (n_beam + 1);
					}
				}
			}
		}
	}
	if (sim_flag == 2)
	{
		int tmp = 0;
		for (int p = 0; p < n_pol; p++)
		{
			for (int f = 0; f < n_chan; f++)
			{
				for (int a = 0; a < nants; a++)
				{
					for (int b = 0; b < n_beam; b++)
					{
						if (tmp >= n_beam)
						{
							tmp = 0;
						}
						tmp = (tmp + 1) % (n_beam + 1);
						coeff_sim[2 * coeff_idx(a, p, b, f, nants, n_pol, n_beam)] = tmp;
					}
				}
			}
		}
	}
	if (sim_flag == 3)
	{
		float c = 3e8;			   // Speed of light
		float c_freq = 1.25e9;	   // Center frequency
		float lambda = c / c_freq; // Wavelength
		float d = lambda / 2;	   // Distance between antennas

		float chan_band = 1; // Fine channel bandwidth in Hz
		double rf_freqs = 0;

		float theta = 0;	// Beam angle from 58 to 122 degrees
		float tau_beam = 0; // Delay

		int b = 2;
		for (int f = 0; f < n_chan; f++)
		{
			rf_freqs = chan_band * f + c_freq;
			theta = ((b - (n_beam / 2)) + 90) * PI / 180; // Beam angle from 58 to 122 degrees - Given SOI at 90 deg or moving across array, the beam with the most power is beamm 33
			tau_beam = d * cos(theta) / c;				  // Delay
			for (int a = 0; a < nants; a++)
			{
				if (n_pol == 1)
				{
					// X polarization
					coeff_sim[2 * coeff_idx(a, 0, b, f, nants, n_pol, n_beam)] = cos(2 * PI * rf_freqs * a * tau_beam);
					coeff_sim[2 * coeff_idx(a, 0, b, f, nants, n_pol, n_beam) + 1] = sin(2 * PI * rf_freqs * a * tau_beam);
				}
				else if (n_pol == 2)
				{
					// X polarization
					coeff_sim[2 * coeff_idx(a, 0, b, f, nants, n_pol, n_beam)] = cos(2 * PI * rf_freqs * a * tau_beam);
					coeff_sim[2 * coeff_idx(a, 0, b, f, nants, n_pol, n_beam) + 1] = sin(2 * PI * rf_freqs * a * tau_beam);
					// Y polarization
					coeff_sim[2 * coeff_idx(a, 1, b, f, nants, n_pol, n_beam)] = cos(2 * PI * rf_freqs * a * tau_beam);
					coeff_sim[2 * coeff_idx(a, 1, b, f, nants, n_pol, n_beam) + 1] = sin(2 * PI * rf_freqs * a * tau_beam);
				}
			}
		}
	}
	if (sim_flag == 4)
	{
		for (int a = 0; a < nants; a++)
		{
			for (int f = 0; f < n_chan; f++)
			{
				coeff_sim[2 * coeff_idx(a, 0, 2, f, nants, n_pol, n_beam)] = 1;
			}
		}
	}

	return coeff_sim;
}