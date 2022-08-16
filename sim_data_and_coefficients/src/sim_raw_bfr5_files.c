#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
//#include <iostream>
#include <string.h>
#include <math.h>
#include "hdf5.h"
#include "sim_data_coeff.h"
#define REC_SIZE 80

#define BASEFILE "guppi_raw_bfr5_test_srcname_JBLAH-BLAH_0000"

int main(int argc, char **argv)
{
	if ((argc > 8) || (argc < 2))
	{
		printf("The minimum requirement is to set the output file as the first argument. Enter -h or --help option for information on how to run this program. \n");
		return -1;
	}
	if (argc > 5)
	{
		printf("Number of beams, polarizations and/or antennas have been chosen.\n");
	}

	if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
	{
		printf("To execute this program enter the following command:\n");
		printf("    ./sim_raw_bfr5_files <RAW and BFR5 file directory> <simulated data flag> <telescope flag> <mode flag or VLASS specifications depending on telescope flag>\n");
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
		printf("        sim_data = 6 -> Simulated drifting signal that simulates ETI given a particular observatory and mode\n");
		printf("    <telescope flag> - Indicate the observatory specifications that you would like to use:\n");
		printf("        MK  -> MeeKAT specifications \n");
		printf("        VLA -> VLA specifications \n");
		printf("    <mode flag> - If MK is selected, then the next argument is the mode, which one of the 3 should be entered as 1k, 4k or 32k. \n");
		printf("    <VLASS specification> - Then if VLA is specified, indicate whether the specifications are the required or desired ones. The required are the default\n");
		printf("    If VLA is specified, the next argument should be input as:\n");
		printf("        required -> Required specifications \n");
		printf("        desired  -> Desired specifications \n");
		printf("An example with a specified simulated data and coefficient files, simulated data flag of <6>, coefficient flag of <4>, telescope flag <MK>, and mode of <4k> is shown below:\n");
		printf("    ./sim_raw_bfr5_files /datag/users/mruzinda/i 6 MK 4k\n");
		printf("If the number beams, polarizations and/or antennas are to be chosen, the following command can be used:\n");
		printf("    ./sim_raw_bfr5_files <RAW and BFR5 file directory> <simulated data flag> <telescope flag> <mode flag or VLASS specifications depending on telescope flag> <number of beams> <number of polarizations> <number of antennas>\n");
		printf("There are limitations to the values of these 3 additional paramters. The max number of polarizations is 2 in any case. \n");
		printf("When the telescope is MK, the max number of beams is 64 and antennas is 64. \n");
		printf("When the telescope is VLA, the max number of beams is 32 and antennas is 32. \n");
		printf("To test the SETI search pipeline, simulated data flag 6 along with coefficient flag 0 or 4 will work best. \n");

		return 0;
	}

	int sim_data_flag = 0;

	// If only the RAW and BFR5 files directory is entered
	if (argc == 2)
	{
		sim_data_flag = 6;
	} // If the files and simulated flags are entered
	else if (argc >= 3)
	{
		sim_data_flag = atoi(argv[2]);
		if (sim_data_flag < 0 || sim_data_flag > 6)
		{
			printf("sim_data_flag is out of bounds i.e. this option doesn't exist. The flag has been set to 5, the default. \n");
			sim_data_flag = 6;
		}
	}

	int telescope_flag = 0;
	int n_subbands = 0;
	// Check for telescope flag
	if (argc > 4)
	{
		if (strcmp(argv[3], "MK") == 0)
		{
			telescope_flag = 0;
			n_subbands = 16;
		}
		else if (strcmp(argv[3], "VLA") == 0)
		{
			telescope_flag = 1;
			n_subbands = 32;
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
	if ((argc > 4) && (strcmp(argv[3], "MK") == 0))
	{
		strcpy(mode_flag, argv[4]);
	} // If VLA is chosen, specify required or desired spcs
	else if ((argc > 4) && (strcmp(argv[3], "VLA") == 0))
	{
		if (strcmp(argv[4], "required") == 0)
		{
			spec_flag = 0;
		}
		else if (strcmp(argv[4], "desired") == 0)
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
	float rect_zero_samps = 0;
	float freq_band_shift = 0;
	float coarse_chan_bw = 0;
	float node_bw = 0;

	// ---------------- MeerKAT specs --------------- //
	if (telescope_flag == 0)
	{
		n_input = N_INPUT;
		node_bw = 13.375; // 13.375 MHz
		if (argc > 5)
		{ // If parameters are specified
			if (argc == 6)
			{
				n_beam = atoi(argv[5]);
				n_pol = 2;
				n_sim_ant = 58;
			}
			else if (argc == 7)
			{
				n_beam = atoi(argv[5]);
				n_pol = atoi(argv[6]);
				n_sim_ant = 58;
			}
			else if (argc == 8)
			{
				n_beam = atoi(argv[5]);
				n_pol = atoi(argv[6]);
				n_sim_ant = atoi(argv[7]);
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
				freq_band_shift = 10000;
				rect_zero_samps = freq_band_shift; // This just happens to workout, but it doesn't have to be this value
				coarse_chan_bw = 0.8359375;		   // 836 kHz
			}									   // 4k mode
			else if (strcmp(mode_flag, "4k") == 0)
			{
				n_chan = 4;			  // 64
				nt = 2 * 1024 * 1024; // 1048576; // 2^20
				freq_band_shift = 10000;
				rect_zero_samps = freq_band_shift; // This just happens to workout, but it doesn't have to be this value
				coarse_chan_bw = 0.208984375;	   // 209 kHz
			}									   // 32k mode
			else if (strcmp(mode_flag, "32k") == 0)
			{
				n_chan = 32;
				nt = 2 * 128 * 1024; // 131072; // 2^17
				freq_band_shift = 100000;
				rect_zero_samps = 1000;
				coarse_chan_bw = 0.02612304687; // 26 kHz
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
				nt = 4096 * 1024;			// 4194304; // 2^22
				freq_band_shift = 3000;		// 10000;
				rect_zero_samps = 60000;	// freq_band_shift; // This just happens to workout, but it doesn't have to be this value
				coarse_chan_bw = 0.8359375; // 836 kHz
			}
			// 4k mode
			else if (strcmp(mode_flag, "4k") == 0)
			{
				n_chan = 4;		  // 64
				nt = 1024 * 1024; // 1048576; // 2^20
				freq_band_shift = 30000;
				rect_zero_samps = 50000;	  // This just happens to workout, but it doesn't have to be this value
				coarse_chan_bw = 0.208984375; // 209 kHz
			}
			// 32k mode
			else if (strcmp(mode_flag, "32k") == 0)
			{
				n_chan = 32;
				nt = 128 * 1024; // 131072; // 2^17
				freq_band_shift = 100000;
				rect_zero_samps = 1000;
				coarse_chan_bw = 0.02612304687; // 26 kHz
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
		coarse_chan_bw = 2; // 2 MHz
		node_bw = 64;		// 64 MHz
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
			freq_band_shift = 3000;	 // 10000;
			rect_zero_samps = 60000; // freq_band_shift;
		}							 // Desired Specification
		else if (spec_flag == 1)
		{
			if (argc > 5)
			{ // If parameters are specified
				if (argc == 6)
				{
					n_beam = atoi(argv[5]);
					n_pol = 2;
					n_sim_ant = 27;
				}
				else if (argc == 7)
				{
					n_beam = atoi(argv[5]);
					n_pol = atoi(argv[6]);
					n_sim_ant = 27;
				}
				else if (argc == 8)
				{
					n_beam = atoi(argv[5]);
					n_pol = atoi(argv[6]);
					n_sim_ant = atoi(argv[7]);
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
			freq_band_shift = 5000;
			rect_zero_samps = 500000; // freq_band_shift;
		}
	}
	// -----------------------------------------------//

	int n_blocks = 128;
	int n_nodes = 64;
	float tbin_num = 1 / coarse_chan_bw;
	int blksize = 2 * n_pol * n_sim_ant * (n_chan * n_subbands) * ((int)nt / n_blocks);
	int obsnchan_num = n_sim_ant * (n_chan * n_subbands);
	int fenchan_num = n_nodes * n_chan * n_subbands;
	int n_samps_per_pkt = 256; // Number of time samples per packet
	int piperblk_num = ((int)nt / n_blocks) / n_samps_per_pkt;
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
	printf("freq_band_shift  = %f (Frequency offset per spectral window)\n", freq_band_shift);
	printf("rect_zero_samps  = %f (Number of time samples that are zero on either side of simulated rect)\n", rect_zero_samps);

	// Generate simulated data
	signed char *sim_data = simulate_data_ubf(n_sim_ant, n_ant_config, n_pol, n_chan, n_samp, n_win, sim_data_flag, telescope_flag, rect_zero_samps, freq_band_shift);
	printf("Simulated data \n");

	// --------------------- Write simulated data to file --------------------- //
	FILE *input_file;

	char instance[256], backend[256], raw_obsid[256], bindhost[256], bindport[256], datadir[256], destip[256], chan_bw[256];
	char obsbw[256], obsfreq[256], hclocks[256], tbin[256], pktstart[256], netstat[256], daqstate[256];
	char blocsize[256], directio[256], nbits[256], npol[256], obsnchan[256], obs_mode[256], diskstat[256], obsinfo_raw[256];
	char fenchan[256], nants[256], schan[256], piperblk[256], synctime[256], pktidx[256], dwell[256], pktstop[256];
	char az[256], ra[256], dec[256], el[256], src_name[256], ra_str[256], dec_str[256], sttvalid[256];
	char stt_imjd[256], stt_smjd[256], stt_offs[256], npkt[256], dropstat[256], end[256];

	char sim_raw_filename[256];

	strcpy(sim_raw_filename, argv[1]);
	strcat(sim_raw_filename, BASEFILE);
	strcat(sim_raw_filename, ".raw");
	printf("Simulated RAW filename = %s\n", sim_raw_filename);

	input_file = fopen(sim_raw_filename, "a");

	if(access(sim_raw_filename, F_OK) != -1){
		if(remove(sim_raw_filename) == 0){
			printf("RAW file being overwritten!\n");
		}else{
			printf("RAW file was not deleted!\n");
		}
		input_file = fopen(sim_raw_filename, "a");
	}else{
		input_file = fopen(sim_raw_filename, "a");

	}

	int char_str_len = 0;

	for (int b = 0; b < n_blocks; b++)
	{
		sprintf(backend, "%-80s", "BACKEND = 'GUPPI'");
		fwrite(backend, sizeof(char), REC_SIZE, input_file);

		sprintf(instance, "%-80s", "INSTANCE= 0");
		fwrite(instance, sizeof(char), REC_SIZE, input_file);

		sprintf(raw_obsid, "%-80s", "OBSID   = 'MK-obsid'");
		fwrite(raw_obsid, sizeof(char), REC_SIZE, input_file);

		sprintf(bindhost, "%-80s", "BINDHOST= 'eth4'");
		fwrite(bindhost, sizeof(char), REC_SIZE, input_file);

		sprintf(bindport, "%-80s", "BINDPORT= 7148");
		fwrite(bindport, sizeof(char), REC_SIZE, input_file);

		sprintf(datadir, "%-80s", "DATADIR = '/datag/users/mruzinda/i'");
		fwrite(datadir, sizeof(char), REC_SIZE, input_file);

		sprintf(destip, "%-80s", "DESTIP  = '239.9.0.192+3'");
		fwrite(destip, sizeof(char), REC_SIZE, input_file);

		sprintf(chan_bw, "CHAN_BW = %.9f                                                           ", coarse_chan_bw);
		fwrite(chan_bw, sizeof(char), REC_SIZE, input_file);

		sprintf(obsbw,   "OBSBW   = %.9f                                                          ", node_bw);
		fwrite(obsbw, sizeof(char), REC_SIZE, input_file);

		sprintf(obsfreq, "%-80s", "OBSFREQ = 1504.5830078125");
		fwrite(obsfreq, sizeof(char), REC_SIZE, input_file);

		sprintf(hclocks, "%-80s", "HCLOCKS = 2097125");
		fwrite(hclocks, sizeof(char), REC_SIZE, input_file);

		sprintf(tbin,    "TBIN    = %.9f                                                           ", tbin_num);
		fwrite(tbin, sizeof(char), REC_SIZE, input_file);

		sprintf(pktstart, "%-80s", "PKTSTART= 0");
		fwrite(pktstart, sizeof(char), REC_SIZE, input_file);

		sprintf(netstat, "%-80s", "NETSTAT = 'receiving'");
		fwrite(netstat, sizeof(char), REC_SIZE, input_file);

		sprintf(daqstate, "%-80s", "DAQSTATE= 'RECORD'");
		fwrite(daqstate, sizeof(char), REC_SIZE, input_file);

		sprintf(blocsize,"BLOCSIZE= %11d                                                           ", blksize);
		fwrite(blocsize, sizeof(char), REC_SIZE, input_file);

		sprintf(directio, "%-80s", "DIRECTIO= 1");
		fwrite(directio, sizeof(char), REC_SIZE, input_file);

		sprintf(nbits, "%-80s", "NBITS   = 8");
		fwrite(nbits, sizeof(char), REC_SIZE, input_file);

		sprintf(npol,    "NPOL    = %11d                                                           ", n_pol);
		fwrite(npol, sizeof(char), REC_SIZE, input_file);

		sprintf(obsnchan,"OBSNCHAN= %11d                                                           ", obsnchan_num);
		fwrite(obsnchan, sizeof(char), REC_SIZE, input_file);

		sprintf(obs_mode, "%-80s", "OBS_MODE= 'RAW'");
		fwrite(obs_mode, sizeof(char), REC_SIZE, input_file);

		sprintf(diskstat, "%-80s", "DISKSTAT= 'waiting'");
		fwrite(diskstat, sizeof(char), REC_SIZE, input_file);

		sprintf(obsinfo_raw, "%-80s", "OBSINFO = 'VALID'");
		fwrite(obsinfo_raw, sizeof(char), REC_SIZE, input_file);

		sprintf(fenchan, "FENCHAN = %11d                                                           ", fenchan_num);
		fwrite(fenchan, sizeof(char), REC_SIZE, input_file);

		sprintf(nants,   "NANTS   = %11d                                                           ", n_sim_ant);
		fwrite(nants, sizeof(char), REC_SIZE, input_file);

		sprintf(schan, "%-80s", "SCHAN   = 0");
		fwrite(schan, sizeof(char), REC_SIZE, input_file);

		sprintf(piperblk,"PIPERBLK= %11d                                                           ", piperblk_num);
		fwrite(piperblk, sizeof(char), REC_SIZE, input_file);

		sprintf(synctime, "%-80s", "SYNCTIME= 160328378");
		fwrite(synctime, sizeof(char), REC_SIZE, input_file);

		sprintf(pktidx,  "PKTIDX  = %11d                                                           ", (b * piperblk_num));
		fwrite(pktidx, sizeof(char), REC_SIZE, input_file);

		sprintf(dwell, "%-80s", "DWELL   = 240");
		fwrite(dwell, sizeof(char), REC_SIZE, input_file);

		sprintf(pktstop, "PKTSTOP = %11d                                                           ", ((n_blocks * piperblk_num) - 1));
		fwrite(pktstop, sizeof(char), REC_SIZE, input_file);

		sprintf(az, "%-80s", "AZ      = 162.23874011012015");
		fwrite(az, sizeof(char), REC_SIZE, input_file);

		sprintf(ra, "%-80s", "RA      = 295.3235416666667");
		fwrite(ra, sizeof(char), REC_SIZE, input_file);

		sprintf(dec, "%-80s", "DEC     = -57.6811111111109");
		fwrite(dec, sizeof(char), REC_SIZE, input_file);

		sprintf(el, "%-80s", "EL      = 60.887789657970366");
		fwrite(el, sizeof(char), REC_SIZE, input_file);

		sprintf(src_name, "%-80s", "SRC_NAME= 'JBORE-SIGHT'");
		fwrite(src_name, sizeof(char), REC_SIZE, input_file);

		sprintf(ra_str, "%-80s", "RA_STR  = '19:41:17.65'");
		fwrite(ra_str, sizeof(char), REC_SIZE, input_file);

		sprintf(dec_str, "%-80s", "DEC_STR = '-57:40:05.2'");
		fwrite(dec_str, sizeof(char), REC_SIZE, input_file);

		sprintf(sttvalid, "%-80s", "STTVALID= 1");
		fwrite(sttvalid, sizeof(char), REC_SIZE, input_file);

		sprintf(stt_imjd, "%-80s", "STT_IMJD= 59143");
		fwrite(stt_imjd, sizeof(char), REC_SIZE, input_file);

		sprintf(stt_smjd, "%-80s", "STT_SMJD= 55142");
		fwrite(stt_smjd, sizeof(char), REC_SIZE, input_file);

		sprintf(stt_offs, "%-80s", "STT_OFFS= 0.32391835500000005");
		fwrite(stt_offs, sizeof(char), REC_SIZE, input_file);

		sprintf(dropstat, "%-80s", "DROPSTAT= '0/118784'");
		fwrite(dropstat, sizeof(char), REC_SIZE, input_file);

		sprintf(end, "%-80s", "END");
		fwrite(end, sizeof(char), REC_SIZE, input_file);

		if(b == 0){
			char_str_len = strlen(backend);
			printf("Backend string length = %d\n", char_str_len);

			char_str_len = strlen(instance);
			printf("Instance string length = %d\n", char_str_len);

			char_str_len = strlen(raw_obsid);
			printf("Obsid string length = %d\n", char_str_len);

			char_str_len = strlen(bindhost);
			printf("Bindhost string length = %d\n", char_str_len);

			char_str_len = strlen(bindport);
			printf("Bindport string length = %d\n", char_str_len);

			char_str_len = strlen(datadir);
			printf("Datadir string length = %d\n", char_str_len);

			char_str_len = strlen(destip);
			printf("Destip string length = %d\n", char_str_len);

			char_str_len = strlen(chan_bw);
			printf("Chan_bw string length = %d\n", char_str_len);

			char_str_len = strlen(obsbw);
			printf("OBSBW string length = %d\n", char_str_len);

			char_str_len = strlen(obsfreq);
			printf("OBSFREQ string length = %d\n", char_str_len);

			char_str_len = strlen(hclocks);
			printf("Hclocks string length = %d\n", char_str_len);

			char_str_len = strlen(tbin);
			printf("tbin string length = %d\n", char_str_len);

			char_str_len = strlen(pktstart);
			printf("pktstart string length = %d\n", char_str_len);

			char_str_len = strlen(netstat);
			printf("netstat string length = %d\n", char_str_len);

			char_str_len = strlen(daqstate);
			printf("daqstate string length = %d\n", char_str_len);

			char_str_len = strlen(blocsize);
			printf("blocsize string length = %d\n", char_str_len);

			char_str_len = strlen(directio);
			printf("directio string length = %d\n", char_str_len);

			char_str_len = strlen(nbits);
			printf("nbits string length = %d\n", char_str_len);

			char_str_len = strlen(npol);
			printf("npol string length = %d\n", char_str_len);

			char_str_len = strlen(obsnchan);
			printf("obsnchan string length = %d\n", char_str_len);

			char_str_len = strlen(obs_mode);
			printf("obs_mode string length = %d\n", char_str_len);

			char_str_len = strlen(diskstat);
			printf("diskstat string length = %d\n", char_str_len);

			char_str_len = strlen(obsinfo_raw);
			printf("obsinfo string length = %d\n", char_str_len);

			char_str_len = strlen(fenchan);
			printf("fenchan string length = %d\n", char_str_len);

			char_str_len = strlen(nants);
			printf("nants string length = %d\n", char_str_len);

			char_str_len = strlen(schan);
			printf("schan string length = %d\n", char_str_len);

			char_str_len = strlen(piperblk);
			printf("piperblk string length = %d\n", char_str_len);

			char_str_len = strlen(synctime);
			printf("synctime string length = %d\n", char_str_len);

			char_str_len = strlen(pktidx);
			printf("pktidx string length = %d\n", char_str_len);

			char_str_len = strlen(dwell);
			printf("dwell string length = %d\n", char_str_len);

			char_str_len = strlen(pktstop);
			printf("pktstop string length = %d\n", char_str_len);

			char_str_len = strlen(az);
			printf("az string length = %d\n", char_str_len);

			char_str_len = strlen(ra);
			printf("ra string length = %d\n", char_str_len);

			char_str_len = strlen(dec);
			printf("dec string length = %d\n", char_str_len);

			char_str_len = strlen(el);
			printf("el string length = %d\n", char_str_len);

			char_str_len = strlen(src_name);
			printf("src_name string length = %d\n", char_str_len);

			char_str_len = strlen(ra_str);
			printf("ra_str string length = %d\n", char_str_len);

			char_str_len = strlen(dec_str);
			printf("dec_str string length = %d\n", char_str_len);

			char_str_len = strlen(sttvalid);
			printf("sttvalid string length = %d\n", char_str_len);

			char_str_len = strlen(stt_imjd);
			printf("stt_imjd string length = %d\n", char_str_len);

			char_str_len = strlen(stt_smjd);
			printf("stt_smjd string length = %d\n", char_str_len);

			char_str_len = strlen(stt_offs);
			printf("stt_offs string length = %d\n", char_str_len);

			char_str_len = strlen(dropstat);
			printf("dropstat string length = %d\n", char_str_len);

			char_str_len = strlen(end);
			printf("end string length = %d\n", char_str_len);
		}

		// Payload
		for (int s = 0; s < n_subbands; s++)
		{
			for (int a = 0; a < n_sim_ant; a++)
			{
				for (int c = 0; c < n_chan; c++)
				{
					fwrite(&sim_data[2 * data_in_idx(0, (((int)nt / n_blocks) * b), 0, a, c, n_pol, nt, 1, n_sim_ant)], sizeof(char), 2 * n_pol * ((int)nt / n_blocks), input_file);
				}
			}
		}

		printf("Block %d/%d written to RAW file.\n", b, n_blocks);

	}

	fclose(input_file);

	// ----------------- Write phase info to BFR5 file ----------------- //
	hid_t file, obsinfo, obsid_dataset, obsid_dataspace, obsid_datatype, calinfo, cal_all_dataset, cal_all_dataspace, cal_all_datatype; /* identifier */
	hid_t beaminfo, ra_dataset, ra_dataspace, ra_datatype;
	hid_t dec_dataset, dec_dataspace, dec_datatype;
	hid_t src_dataset, src_dataspace, native_src_type, src_datatype;
	hid_t delayinfo, delays_dataset, delays_dataspace;
	hid_t time_array_dataset, time_array_dataspace;
	hsize_t cal_all_dims[3]; /* cal_all dataset dimensions */
	hsize_t ra_dims;		 /* ra dataset dimensions */
	hsize_t dec_dims;		 /* dec dataset dimensions */
	hsize_t src_dims;		 /* src name dataset dimensions */
	hsize_t obsid_dims;		 /* obsid dataset dimensions */
	hsize_t delays_dims[3];	 /* delays dataset dimensions */
	hsize_t time_array_dims; /* time_array dataset dimensions */
	herr_t status;
	int delays_elements = NTIMES_BFR5 * NBEAMS_BFR5 * NANTS_BFR5;

	hid_t reim_tid;
	reim_tid = H5Tcreate(H5T_COMPOUND, sizeof(complex_t));
	H5Tinsert(reim_tid, "r", HOFFSET(complex_t, re), H5T_IEEE_F32LE);
	H5Tinsert(reim_tid, "i", HOFFSET(complex_t, im), H5T_IEEE_F32LE);

	// Create string data type for src name
	native_src_type = H5Tvlen_create(H5T_NATIVE_CHAR);

	complex_t cal_all_data[NCHAN_BFR5][NPOL_BFR5][NANTS_BFR5];
	// Calibration solutions are set to 1 since it'll be very difficult to simulate them
	for (int i = 0; i < NCHAN_BFR5; i++)
	{
		for (int j = 0; j < NPOL_BFR5; j++)
		{
			for (int k = 0; k < NANTS_BFR5; k++)
			{
				cal_all_data[i][j][k].re = 1;
			}
		}
	}
	double delays_data[NTIMES_BFR5][NBEAMS_BFR5][NANTS_BFR5];
	double *time_array_data;
	double *ra_data;
	double *dec_data;
	uint64_t nbeams;
	hvl_t *src_names_str;

	// Allocate memory for array
	// cal_all_data = malloc((int)cal_all_elements * sizeof(complex_t));
	// delays_data = malloc(delays_elements * sizeof(double));
	time_array_data = malloc(NTIMES_BFR5 * sizeof(double));
	ra_data = malloc(NBEAMS_BFR5 * sizeof(double));
	dec_data = malloc(NBEAMS_BFR5 * sizeof(double));
	src_names_str = malloc(NBEAMS_BFR5 * sizeof(hvl_t));

	char b_str[256] = {0};
	char src_name_tmp[256];
	for (int i = 0; i < NBEAMS_BFR5; i++)
	{
		sprintf(b_str, "%02d", i);
		strcpy(src_name_tmp, "JBLAH-BLAH.B");
		strcat(src_name_tmp, b_str);
		src_names_str[i].p = src_name_tmp;
		src_names_str[i].len = strlen(src_names_str[i].p);
		printf("%d: len: %d, str is: %s\n", i, (int)src_names_str[i].len, (char *)src_names_str[i].p);
	}

	char hdf5_obsid[256];
	strcpy(hdf5_obsid, "MK-obsid");

	int Nant = 63;	 // Number of antennas
	int Nbeams = 61; // Number of beams
	int Ntimes = 30; // Number of time stamps

	/*
	 * Create a new file using H5ACC_TRUNC access,
	 * default file creation properties, and default file
	 * access properties.
	 * Then close the file.
	 */
	char bfr5_filename[256];

	strcpy(bfr5_filename, argv[1]);
	strcat(bfr5_filename, BASEFILE);
	strcat(bfr5_filename, ".bfr5");
	printf("Simulated BFR5 filename = %s\n", bfr5_filename);

	file = H5Fcreate(bfr5_filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	/*
	 * Create a group in the file.
	 */
	calinfo = H5Gcreate(file, "/calinfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	beaminfo = H5Gcreate(file, "/beaminfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	delayinfo = H5Gcreate(file, "/delayinfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	obsinfo = H5Gcreate(file, "/obsinfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	/*
	 * Describe the size of the array and create the data space for fixed
	 * size dataset.
	 */
	cal_all_dims[0] = NCHAN_BFR5;
	cal_all_dims[1] = NPOL_BFR5;
	cal_all_dims[2] = NANTS_BFR5;
	cal_all_dataspace = H5Screate_simple(3, cal_all_dims, NULL);

	delays_dims[0] = NTIMES_BFR5;
	delays_dims[1] = NBEAMS_BFR5;
	delays_dims[2] = NANTS_BFR5;
	delays_dataspace = H5Screate_simple(3, delays_dims, NULL);

	time_array_dims = NTIMES_BFR5;
	time_array_dataspace = H5Screate_simple(1, &time_array_dims, NULL);

	ra_dims = NBEAMS_BFR5;
	ra_dataspace = H5Screate_simple(1, &ra_dims, NULL);

	dec_dims = NBEAMS_BFR5;
	dec_dataspace = H5Screate_simple(1, &dec_dims, NULL);

	src_dims = NBEAMS_BFR5;
	src_dataspace = H5Screate_simple(1, &src_dims, NULL);

	obsid_dims = 1;
	obsid_dataspace = H5Screate_simple(1, &obsid_dims, NULL);

	/*
	 * Define datatype for the data in the file.
	 * We will store little endian INT numbers.
	 */
	cal_all_datatype = H5Tcopy(reim_tid);
	status = H5Tset_order(cal_all_datatype, H5T_ORDER_LE);

	src_datatype = H5Tcopy(native_src_type);
	status = H5Tset_order(src_datatype, H5T_ORDER_LE);

	obsid_datatype = H5Tcopy(H5T_NATIVE_CHAR);
	status = H5Tset_order(obsid_datatype, H5T_ORDER_LE);

	/*
	 * Create a new dataset within the file using defined dataspace and
	 * datatype and default dataset creation properties.
	 */
	cal_all_dataset = H5Dcreate(file, "/calinfo/cal_all", cal_all_datatype, cal_all_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	delays_dataset = H5Dcreate(file, "/delayinfo/delays", H5T_IEEE_F64LE, delays_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	time_array_dataset = H5Dcreate(file, "/delayinfo/time_array", H5T_IEEE_F64LE, time_array_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	ra_dataset = H5Dcreate(file, "/beaminfo/ras", H5T_IEEE_F64LE, ra_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	dec_dataset = H5Dcreate(file, "/beaminfo/decs", H5T_IEEE_F64LE, dec_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	src_dataset = H5Dcreate(file, "/beaminfo/src_names", src_datatype, src_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	obsid_dataset = H5Dcreate(file, "/obsinfo/obsid", obsid_datatype, obsid_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	/*
	 * Write the data to the dataset using default transfer properties.
	 */
	status = H5Dwrite(cal_all_dataset, reim_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, cal_all_data);
	status = H5Dwrite(delays_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, delays_data);
	status = H5Dwrite(time_array_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, time_array_data);
	status = H5Dwrite(ra_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, ra_data);
	status = H5Dwrite(dec_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dec_data);
	status = H5Dwrite(src_dataset, native_src_type, H5S_ALL, H5S_ALL, H5P_DEFAULT, src_names_str);
	status = H5Dwrite(obsid_dataset, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, hdf5_obsid);

	/*
	 * Close/release resources
	 */
	status = H5Sclose(cal_all_dataspace);
	status = H5Tclose(cal_all_datatype);
	status = H5Dclose(cal_all_dataset);

	status = H5Sclose(delays_dataspace);
	status = H5Dclose(delays_dataset);

	status = H5Sclose(time_array_dataspace);
	status = H5Dclose(time_array_dataset);

	status = H5Sclose(ra_dataspace);
	status = H5Dclose(ra_dataset);

	status = H5Sclose(dec_dataspace);
	status = H5Dclose(dec_dataset);

	status = H5Sclose(src_dataspace);
	status = H5Tclose(src_datatype);
	status = H5Dclose(src_dataset);

	status = H5Sclose(obsid_dataspace);
	status = H5Tclose(obsid_datatype);
	status = H5Dclose(obsid_dataset);

	status = H5Fclose(file);

	return 0;
}