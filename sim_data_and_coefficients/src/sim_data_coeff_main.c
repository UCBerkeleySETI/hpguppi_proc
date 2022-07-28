#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
//#include <iostream>
#include <string.h>
#include <math.h>
#include "sim_data_coeff.h"

int main(int argc, char **argv)
{
	if ((argc > 10) || (argc < 2))
	{
		printf("The minimum requirement is to set the output file as the first argument. Enter -h or --help option for information on how to run this program. \n");
		return -1;
	}
	if (argc > 7)
	{
		printf("Number of beams, polarizations and/or antennas have been chosen.\n");
	}

	if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
	{
		printf("To execute this program enter the following command:\n");
		printf("    ./sim_data_coeff <simulated data file name> <coefficient file name> <simulated data flag> <simulated coefficients flag> <telescope flag> <mode flag or VLASS specifications depending on telescope flag>\n");
		printf("    <> are not used in the command, but are just used to indicate arguments in this description\n");
		printf("Descriptions of the arguments in the command above:\n");
		printf("    <output file> - Enter the binary file along with it's path e.g. /datag/users/mruzinda/o/output_d_fft_bf.bin \n");
		printf("    The minimum requirement is to set the output file as the first argument. \n");
		printf("    <simulated data flag> - Enter the flag for the kind of simulated data that you would like to use. Default is 0. The following are the options:\n");
		printf("        sim_data = 0 -> Ones (Default)\n");
		printf("        sim_data = 1 -> Ones placed in a particular bin (bin 3 for now)\n");
		printf("        sim_data = 2 -> Ones placed in a particular bin at a particular antenna (bin 3 and antenna 3 for now)\n");
		printf("        sim_data = 3 -> Rect placed in a particular bin at a particular antenna (bin 3 and antenna 3 for now)\n");
		printf("        sim_data = 4 -> Simulated cosine wave\n");
		printf("        sim_data = 5 -> Simulated complex exponential i.e. exp(j*2*pi*f0*t)\n");
		printf("        sim_data = 6 -> Simulated drifting signal or complex exponential i.e. exp(j*2*pi*f0*t)\n");
		printf("    <simulated coefficients flag> - Enter the flag for the kind of simulated coefficients that you would like to use. Default is 0. The following are the options:\n");
		printf("        sim_coef = 0 -> Ones (Default)\n");
		printf("        sim_coef = 1 -> Scale each beam by incrementing value i.e. beam 1 = 1, beam 2 = 2, ..., beam 64 = 64\n");
		printf("        sim_coef = 2 -> Scale each beam by incrementing value in a particular bin (bin 3 and 6 for now). Match simulated data sim_flag = 2\n");
		printf("        sim_coef = 3 -> Simulated beams from 58 to 122 degrees. Assuming a ULA.\n");
		printf("        sim_coef = 4 -> One value at one polarization, one element, one beam, and one frequency bin\n");
		printf("    <telescope flag> - Indicate the observatory specifications that you would like to use:\n");
		printf("        MK  -> MeeKAT specifications \n");
		printf("        VLA -> VLA specifications \n");
		printf("    <mode flag> - If MK is selected, then the next argument is the mode, which one of the 3 should be entered as 1k, 4k or 32k. \n");
		printf("    <VLASS specification> - Then if VLA is specified, indicate whether the specifications are the required or desired ones. The required are the default\n");
		printf("    If VLA is specified, the next argument should be input as:\n");
		printf("        required -> Required specifications \n");
		printf("        desired  -> Desired specifications \n");
		printf("An example with a specified simulated data and coefficient files, simulated data flag of <5>, coefficient flag of <4>, telescope flag <MK>, and mode of <4k> is shown below:\n");
		printf("    ./sim_data_coeff /datag/users/mruzinda/i/sim_data.bin /datag/users/mruzinda/i/sim_coeff.bin 5 4 MK 4k\n");
		printf("If the number beams, polarizations and/or antennas are to be chosen, the following command can be used:\n");
		printf("    ./sim_data_coeff <simulated data file name> <coefficient file name> <simulated data flag> <simulated coefficients flag> <telescope flag> <mode flag or VLASS specifications depending on telescope flag> <number of beams> <number of polarizations> <number of antennas>\n");
		printf("There are limitations to the values of these 3 additional paramters. The max number of polarizations is 2 in any case. \n");
		printf("When the telescope is MK, the max number of beams is 64 and antennas is 64. \n");
		printf("When the telescope is VLA, the max number of beams is 32 and antennas is 32. \n");

		return 0;
	}

	int sim_data_flag = 0;
	int sim_coef_flag = 0;

	// If only the simulated data and coefficient files are entered
	if (argc == 3)
	{
		sim_data_flag = 5;
		sim_coef_flag = 4;
	} // If the files and another argument are entered
	else if (argc == 4)
	{
		sim_data_flag = atoi(argv[3]);
		if (sim_data_flag < 0 || sim_data_flag > 6)
		{
			printf("sim_data_flag is out of bounds i.e. this option doesn't exist. The flag has been set to 5, the default. \n");
			sim_data_flag = 5;
		}
		sim_coef_flag = 4;
	} // If the files and simulated flags are entered
	else if (argc >= 5)
	{
		sim_data_flag = atoi(argv[3]);
		sim_coef_flag = atoi(argv[4]);
		if (sim_data_flag < 0 || sim_data_flag > 6)
		{
			printf("sim_data_flag is out of bounds i.e. this option doesn't exist. The flag has been set to 5, the default. \n");
			sim_data_flag = 5;
		}
		if (sim_coef_flag < 0 || sim_coef_flag > 4)
		{
			printf("sim_coef_flag is out of bounds i.e. this option doesn't exist. The flag has been set to 0, the default. \n");
			sim_coef_flag = 4;
		}
	}

	int telescope_flag = 0;
	// Check for telescope flag
	if (argc > 5)
	{
		if (strcmp(argv[5], "MK") == 0)
		{
			telescope_flag = 0;
		}
		else if (strcmp(argv[5], "VLA") == 0)
		{
			telescope_flag = 1;
		}
	} // Default telescope
	else
	{
		printf("The observatory was not entered. The default is MK -> MeerKAT.\n");
		printf("Enter -h as argument for help.\n");
		telescope_flag = 0;
	}

	char mode_flag[5]; // Flag for operational mode for MeerKAT
	int spec_flag = 0; // Specification flag for VLASS
	// If MK is chosen, also get the mode
	if ((argc > 6) && (strcmp(argv[5], "MK") == 0))
	{
		strcpy(mode_flag, argv[6]);
	} // If VLA is chosen, specify required or desired spcs
	else if ((argc > 6) && (strcmp(argv[5], "VLA") == 0))
	{
		if (strcmp(argv[6], "required") == 0)
		{
			spec_flag = 0;
		}
		else if (strcmp(argv[6], "desired") == 0)
		{
			spec_flag = 1;
		}
		else
		{
			printf("Incorrect option, enter <required> or <desired>. The default is with <required> specifications for VLASS.\n");
			printf("Enter -h as argument for help.\n");
			spec_flag = 0;
		}
	} // Default mode
	else
	{
		printf("4k mode is the default with MeerKAT chosen or if the telescope is not specified. \n");
		strcpy(mode_flag, "4k");
	}

	printf("Mode = %s \n", mode_flag);
	char sim_data_filename[128];
	char sim_coeff_filename[128];

	strcpy(sim_data_filename, argv[1]);
	printf("Simulated data filename = %s\n", sim_data_filename);
	strcpy(sim_coeff_filename, argv[2]);
	printf("Simulated coefficient filename = %s\n", sim_coeff_filename);
	// strcpy(output_filename, "/datag/users/mruzinda/o/output_d_fft_bf.bin");
	// strcpy(output_filename, "/mydatag/Unknown/GUPPI/output_d_fft_bf.bin");
	// strcpy(output_filename, "/home/mruzinda/tmp_output/output_d_fft_bf.bin");

	// ---------------------------- //
	// To run in regular array configuration, enter values between 33 and 64 in n_beam and n_ant
	// To run in subarray configuration, enter values 32 or less (and greater than 1 otherwise, beamforming can't be done)
	// ---------------------------- //
	int n_beam = 0;
	int n_pol = 0;
	int n_sim_ant = 0;
	int n_ant_config = 0;
	int n_chan = 0;
	int nt = 0;
	int n_win = 0;
	int n_time_int = 0;
	int n_input = 0;
	int n_coeff = 0;

	// ---------------- MeerKAT specs --------------- //
	if (telescope_flag == 0)
	{
		n_input = N_INPUT;
		n_coeff = N_COEFF;
		if (argc > 7)
		{ // If parameters are specified
			if (argc == 8)
			{
				n_beam = atoi(argv[7]);
				n_pol = 2;
				n_sim_ant = 58;
			}
			else if (argc == 9)
			{
				n_beam = atoi(argv[7]);
				n_pol = atoi(argv[8]);
				n_sim_ant = 58;
			}
			else if (argc == 10)
			{
				n_beam = atoi(argv[7]);
				n_pol = atoi(argv[8]);
				n_sim_ant = atoi(argv[9]);
			}
		}
		else
		{ // Default parameters
			printf("Default parameters used! \n");
			n_beam = 61;
			n_pol = 2;
			n_sim_ant = 58;
		}
		if (n_sim_ant <= N_ANT / 2)
		{ // Subarray configuration
			n_ant_config = N_ANT / 2;
			// 5 seconds worth of processing at a time
			// 1k mode
			if (strcmp(mode_flag, "1k") == 0)
			{
				n_chan = 1;
				nt = 2 * 4096 * 1024; // 4194304; // 2^22
			}						  // 4k mode
			else if (strcmp(mode_flag, "4k") == 0)
			{
				n_chan = 4;			  // 64
				nt = 2 * 1024 * 1024; // 1048576; // 2^20
			}						  // 32k mode
			else if (strcmp(mode_flag, "32k") == 0)
			{
				n_chan = 32;
				nt = 2 * 128 * 1024; // 131072; // 2^17
			}

			n_win = 16;
			n_time_int = 16;
		}
		else
		{ // Regular array configuration
			n_ant_config = N_ANT;
			// 5 seconds worth of processing at a time
			// 1k mode
			if (strcmp(mode_flag, "1k") == 0)
			{
				n_chan = 1;
				nt = 4096 * 1024; // 4194304; // 2^22
			}					  // 4k mode
			else if (strcmp(mode_flag, "4k") == 0)
			{
				n_chan = 4;		  // 64
				nt = 1024 * 1024; // 1048576; // 2^20
			}					  // 32k mode
			else if (strcmp(mode_flag, "32k") == 0)
			{
				n_chan = 32;
				nt = 128 * 1024; // 131072; // 2^17
			}
			n_win = 8;
			n_time_int = 8;
		}
	}
	// -----------------------------------------------//
	// ------------------ VLASS specs ----------------//
	else if (telescope_flag == 1)
	{
		n_input = VLASS_N_INPUT;
		n_coeff = VLASS_N_COEFF;
		// Required Specification
		if (spec_flag == 0)
		{
			printf("Required mode chosen. Specifications remain unchanged from default. \n");
			n_beam = 5;
			n_pol = 2;
			n_sim_ant = 27;
			n_ant_config = N_ANT / 2;
			n_chan = 1;
			nt = 5013504; // 5120000;
			n_win = 32;	  // 40
			n_time_int = 1;
		} // Desired Specification
		else if (spec_flag == 1)
		{
			if (argc > 7)
			{ // If parameters are specified
				if (argc == 8)
				{
					n_beam = atoi(argv[7]);
					n_pol = 2;
					n_sim_ant = 27;
				}
				else if (argc == 9)
				{
					n_beam = atoi(argv[7]);
					n_pol = atoi(argv[8]);
					n_sim_ant = 27;
				}
				else if (argc == 10)
				{
					n_beam = atoi(argv[7]);
					n_pol = atoi(argv[8]);
					n_sim_ant = atoi(argv[9]);
				}
			}
			else
			{ // Default parameters
				printf("Default parameters used! \n");
				n_beam = 31;
				n_pol = 2;
				n_sim_ant = 27;
			}
			n_ant_config = N_ANT / 2;
			n_chan = 1;
			nt = 5013504; // 10240000; // 5120000
			n_win = 4;	  // 10000; // 2, 80, 10000
			n_time_int = 1;
		}
	}
	// -----------------------------------------------//

	int n_samp = nt / n_win;
	int n_sti = n_win / n_time_int;

	printf("Specifications are as follows:\n");
	printf("n_beam = %d\n", n_beam);
	printf("n_pol  = %d\n", n_pol);
	printf("n_ant  = %d\n", n_sim_ant);
	printf("n_freq = %d (Number of coarse channels)\n", n_chan);
	printf("n_time = %d (Number of time samples)\n", nt);
	printf("n_fft  = %d (Number of points in FFT)\n", n_samp);
	printf("n_win  = %d (Number of spectral windows after FFT)\n", n_win);
	printf("n_int  = %d (Number of integrated windows)\n", n_time_int);

	// Generate simulated data
	signed char *sim_data = simulate_data_ubf(n_sim_ant, n_ant_config, n_pol, n_chan, n_samp, n_win, sim_data_flag, telescope_flag);
	printf("Simulated data \n");

	// Generate simulated weights or coefficients
	float *sim_coefficients = simulate_coefficients_ubf(n_sim_ant, n_ant_config, n_pol, n_beam, n_chan, sim_coef_flag, telescope_flag);
	printf("Simulated coefficients \n");

	// --------------------- Write simulated data to file --------------------- //
	FILE *input_file;

	input_file = fopen(sim_data_filename, "w");

	fwrite(sim_data, sizeof(signed char), n_input, input_file);

	fclose(input_file);

	// ----------------- Write simulated coefficients to file ----------------- //
	FILE *coeff_file;

	coeff_file = fopen(sim_coeff_filename, "w");

	fwrite(sim_coefficients, sizeof(float), n_coeff, coeff_file);

	fclose(coeff_file);

	return 0;
}