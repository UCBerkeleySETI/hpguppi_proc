/* hpguppi_coherent_bf_katpoint_thread_fb.c
 * 
 * Gets delays using named pipe in conjuction with python script that utilizes katpoint library.
 * Perform coherent beamforming and write databuf blocks out to filterbank files on disk.
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <stdint.h>
#include <endian.h>
#include <math.h>

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include "ioprio.h"

//#include "hpguppi_time.h"
#include "coherent_beamformer_char_in.h"

#include "hashpipe.h"

// Use rawpsec_fbutils because it contains all of the necessary functions to write headers to filterbank files
// This might change in the near future to make this library completely separate from rawspec
#include "rawspec_fbutils.h"
// Use rawpsec_rawutils because it contains all of the necessary functions to parse raw headers orginally from GUPPI RAW files
// This might change in the near future to make this library completely separate from rawspec
#include "rawspec_rawutils.h"

#include "hpguppi_databuf.h"
#include "hpguppi_params.h"
//#include "hpguppi_pksuwl.h"
#include "hpguppi_util.h"

// 80 character string for the BACKEND header record.
static const char BACKEND_RECORD[] =
// 0000000000111111111122222222223333333333
// 0123456789012345678901234567890123456789
  "BACKEND = 'GUPPI   '                    " \
  "                                        ";

static int safe_close_all(int *pfd) {
  if (pfd==NULL) return 0;
  int pfd_size = sizeof(pfd)/sizeof(int);
  int fd_val = -1;
  for(int i = 0; i < pfd_size; i++){
     if(pfd[i] == -1){
       fsync(pfd[i]);
       fd_val = close(pfd[i]);
       if(fd_val == -1){
         printf("A file was not successfully closed! \n");
       }
     }
  }
  return 0;
}

void
update_fb_hdrs_from_raw_hdr_cbf(fb_hdr_t fb_hdr, const char *p_rawhdr)
{
  rawspec_raw_hdr_t raw_hdr;

  rawspec_raw_parse_header(p_rawhdr, &raw_hdr);
  hashpipe_info(__FUNCTION__,
      "beam_id = %d/%d", raw_hdr.beam_id, raw_hdr.nbeam);

  // Update filterbank headers based on raw params and Nts etc.
  // Same for all products
  fb_hdr.telescope_id = fb_telescope_id(raw_hdr.telescop);
  fb_hdr.src_raj = raw_hdr.ra;
  fb_hdr.src_dej = raw_hdr.dec;
  fb_hdr.tstart = raw_hdr.mjd;
  fb_hdr.ibeam = raw_hdr.beam_id;
  fb_hdr.nbeams = raw_hdr.nbeam;
  strncpy(fb_hdr.source_name, raw_hdr.src_name, 80);
  fb_hdr.source_name[80] = '\0';
  // Output product dependent
  fb_hdr.foff = raw_hdr.obsbw/raw_hdr.obsnchan/N_TIME;
  // This computes correct fch1 for odd or even number of fine channels
  fb_hdr.fch1 = raw_hdr.obsfreq
    - raw_hdr.obsbw*(raw_hdr.obsnchan-1)/(2*raw_hdr.obsnchan)
    - (N_TIME/2) * fb_hdr.foff
    ;//TODO + schan * raw_hdr.obsbw / raw_hdr.obsnchan; // Adjust for schan
  //fb_hdr.nchans = 16; // 1k mode
  fb_hdr.nchans = 64; // 4k mode
  //fb_hdr.nchans = 512; // 32k mode
  fb_hdr.tsamp = raw_hdr.tbin * N_TIME * 256*1024;
  // TODO az_start, za_start
}

static void *run(hashpipe_thread_args_t * args)
{
  // Local aliases to shorten access to args fields
  // Our output buffer happens to be a hpguppi_input_databuf
  hpguppi_input_databuf_t *db = (hpguppi_input_databuf_t *)args->ibuf;
  hashpipe_status_t *st = &args->st;
  const char * thread_name = args->thread_desc->name;
  const char * status_key = args->thread_desc->skey;

  /* Read in general parameters */
  struct hpguppi_params gp;
  struct psrfits pf;
  pf.sub.dat_freqs = NULL;
  pf.sub.dat_weights = NULL;
  pf.sub.dat_offsets = NULL;
  pf.sub.dat_scales = NULL;
  pthread_cleanup_push((void *)hpguppi_free_psrfits, &pf);

  /* Init output file descriptor (-1 means no file open) */
  static int fdraw[N_BEAM];
  memset(fdraw, -1, N_BEAM*sizeof(int));
  pthread_cleanup_push((void *)safe_close_all, fdraw);

  // Set I/O priority class for this thread to "real time" 
  if(ioprio_set(IOPRIO_WHO_PROCESS, 0, IOPRIO_PRIO_VALUE(IOPRIO_CLASS_RT, 7))) {
    hashpipe_error(thread_name, "ioprio_set IOPRIO_CLASS_RT");
  }

  printf("CBF: After ioprio_set...\n");

  /* Loop */
  int64_t pktidx=0, pktstart=0, pktstop=0;
  int blocksize=0; // Size of beamformer output (output block size)
  int curblock=0;
  int block_count=0, filenum=0, next_filenum=0;
  int got_packet_0=0, first=1;
  char *ptr;
  int open_flags = 0;
  int rv = 0;
  double obsbw;
  double tbin;
  double obsfreq;
  char basefilename[200];
  char outdir[200];
  char character = '/';
  char *char_offset; 
  long int slash_pos;
  char raw_basefilename[200];
  char new_base[200];
  //int header_size = 0;

  // Filterbank file header initialization
  fb_hdr_t fb_hdr;
  fb_hdr.machine_id = 20;
  fb_hdr.telescope_id = -1; // Unknown (updated later)
  fb_hdr.data_type = 1;
  fb_hdr.nbeams =  1;
  fb_hdr.ibeam  =  -1; //Not used
  fb_hdr.nbits  = 32;
  fb_hdr.nifs   = 1;

  // Timing variables
  float bf_time = 0;
  float write_time = 0;
  float time_taken = 0;
  float time_taken_w = 0;

  // --------------------- Initial delay calculations with katpoint and mosaic --------------------------------//
  // Or all calculations may be done in the while loop below
  // File descriptor
  int fdr;
  int fdc;
  int fde;

  // Return value of read() function
  int read_val;

  // Return value of write() function
  int wvale;
  int wvalc;

  // Array of floats to place data read from file
  float delay_pols[N_DELAYS];
  
  printf("CBF: After variable and pointer initializations ...\n");

  // FIFO file path
  char * myfifo = "/tmp/katpoint_delays";
  char * myfifo_e= "/tmp/epoch";
  char * myfifo_c= "/tmp/obsfreq";

  printf("CBF: After mkfifos...\n");
  // ------------------------------------------------------------------------------------------------//

  
  uint64_t synctime = 0;
  uint64_t hclocks = 1;
  uint32_t fenchan = 1;
  double chan_bw = 1.0;
  uint64_t nants = 0;
  uint64_t obsnchan = 0;
  uint64_t npol = 0;
  uint64_t blksize = 0; // Raw file block size
  int nt = 0; // Number of time samples per block in a RAW file
  int ns = 0; // Number of STI windows in output
  //int n_telstate_phase = 0;
  double realtime_secs = 0.0; // Real-time in seconds according to the RAW file metadata
  double epoch_sec = 0.0; // Epoch used for delay polynomial

  // The number of blocks in a RAW file (128 blocks) should be divisible by n_calc_blks
  // n_calc_blks is the duration of the delay polynomial calculation e.g. 128 blocks is approx. 5 seconds
  // Currently, n_update_blks should not exceed 481 ms as stated by FBFUSE which is approximately 12 blocks
  int n_calc_blks = 128; // Number of blocks to calculate delay polynomials
  int n_update_blks = 3; // Number of blocks to update coefficients with new epoch
  
  float* bf_coefficients; // Beamformer coefficients
  float* tmp_coefficients; // Temporary coefficients
  //float* telstate_phase[N_PHASE*N_FREQ]; // Telstate phase solutions

  // Frequency parameter initializations
  float full_bw = 856e6; // Hz
  float coarse_chan_band = 0; // Coarse channel bandwidth
  double coarse_chan_freq[N_FREQ]; // Coarse channel center frequencies in the band 
  int n_nodes = 64; // Number of compute nodes
  int n_chan_per_node = 0; // Number of coarse channels per compute node

  int sim_flag = 0; // Flag to use simulated coefficients (set to 1) or calculated beamformer coefficients (set to 0)
  // Add if statement for generate_coefficients() function option which has 3 arguments - tau, coarse frequency channel, and epoch
  if(sim_flag == 1){
    // Generate weights or coefficients (using simulate_coefficients() for now)
    // Used with simulated data when no RAW file is read
    //int n_chan = 16;  // 1k mode
    int n_chan = 64;  // 4k mode
    //int n_chan = 512; // 32k mode
    tmp_coefficients = simulate_coefficients(n_chan);
    // Register the array in pinned memory to speed HtoD mem copy
    coeff_pin(tmp_coefficients);
  }
  if(sim_flag == 0){
    bf_coefficients = (float*)calloc(N_COEFF, sizeof(float)); // Beamformer coefficients
    coeff_pin(bf_coefficients);
  }

  // -----------------Get phase solutions----------------- //

  // Make all initializations before while loop
  // Initialize beamformer (allocate all memory on the device)
  printf("CBF: Initializing beamformer...\n");
  init_beamformer();

  // Initialize output data array
  float* output_data;

  printf("CBF: Using host arrays allocated in pinned memory\n\r");
  for (int i = 0; i < N_INPUT_BLOCKS; i++){
    ///////////////////////////////////////////////////////////////////////////////
    //>>>>   Register host array in pinned memory <<<<
    ///////////////////////////////////////////////////////////////////////////////
    input_data_pin((signed char *)&db->block[i].data);
  }

  while (run_threads()) {

    /* Note waiting status */
    hashpipe_status_lock_safe(st);
    hputs(st->buf, status_key, "waiting");
    hashpipe_status_unlock_safe(st);

    /* Wait for buf to have data */
    rv = hpguppi_input_databuf_wait_filled(db, curblock);
    if (rv!=0) continue;

    /* Read param struct for this block */
    ptr = hpguppi_databuf_header(db, curblock);
    if (first) {
      hpguppi_read_obs_params(ptr, &gp, &pf);
      first = 0;
    } else {
      hpguppi_read_subint_params(ptr, &gp, &pf);
    }

    /* Read pktidx, pktstart, pktstop from header */
    hgeti8(ptr, "PKTIDX", &pktidx);
    hgeti8(ptr, "PKTSTART", &pktstart);
    hgeti8(ptr, "PKTSTOP", &pktstop);

    /* Get values for calculations at varying points in processing */
    hgetu8(ptr, "SYNCTIME", &synctime);
    hgetu8(ptr, "HCLOCKS", &hclocks);
    hgetu4(ptr, "FENCHAN", &fenchan);
    hgetr8(ptr, "CHAN_BW", &chan_bw);
    hgetr8(ptr, "OBSFREQ", &obsfreq);
    hgetu8(ptr, "NANTS", &nants);
    hgetu8(ptr, "BLOCSIZE", &blksize); // Raw file block size
    hgetu8(ptr, "OBSNCHAN", &obsnchan);
    hgetu8(ptr, "NPOL", &npol);

    if(filenum == next_filenum){
      next_filenum++;

      // Number of time samples per block in a RAW file
      nt = (int)(blksize/(2*obsnchan*npol));

      printf("CBF: Got center frequency, obsfreq = %lf MHz and n. time samples = %d \n", obsfreq, nt);

      // Calculate coarse channel center frequencies depending on the mode and center frequency that spans the RAW file
      coarse_chan_band = full_bw/fenchan;
      n_chan_per_node = ((int)fenchan)/n_nodes;
      blocksize=((N_BF_POW*n_chan_per_node)/(N_BEAM*MAX_COARSE_FREQ))*sizeof(float); // Size of beamformer output

      // Skip zeroth index since the number of coarse channels is even and the center frequency is between the 2 middle channels
      for(int i=0; i<(n_chan_per_node/2); i++){
        coarse_chan_freq[i] = (i-(n_chan_per_node/2))*coarse_chan_band + (obsfreq*1e6);
      }
      for(int i=(n_chan_per_node/2); i<n_chan_per_node; i++){
        coarse_chan_freq[i] = ((i+1)-(n_chan_per_node/2))*coarse_chan_band + (obsfreq*1e6);
      }

      // Check Redis for phase solution by comparing timestamp there with timestamp calculated with synctime from raw file
      // Make sure the timestamp is converted to the mjd format used in the hpguppi_meerkat_spead_thread.c script
      //n_telstate_phase = N_PHASE*n_chan_per_node;

      // Get the phase solutions and possibly copy the values to the telstate_phase array.
      // Update by comparing the timestamps

      // Write to new FIFO going to get_delays module for delay polynomial calculation
      // Creating the named file(FIFO)
      // mkfifo(<pathname>,<permission>)
      mkfifo(myfifo_c, 0666);

      while(access(myfifo_c, F_OK) == 0){
        // If the FIFO has been deleted/removed by the get_delays module,
        // then that means it has been read and we can go retrieve the delay polynomials
        if(access(myfifo_c, F_OK) != 0){
          printf("CBF: FIFO used to write center frequency to get_delays module was deleted...\n");
          break;
        }

        // Open file as a read/write
        fdc = open(myfifo_c,O_RDWR);
        if(fdc == -1){
          printf("CBF: Unable to open fifo_c file...\n");
          break;
        }

        //printf("CBF: After open(myfifo_c)...\n");

        // Write to file
        wvalc = write(fdc, &obsfreq, sizeof(obsfreq));
        if(wvalc != 0){
          printf("CBF: Written to obsfreq FIFO!\n");
        }

        //printf("CBF: After write(myfifo_c)...\n");

        // Close file
        close(fdc);
      }
    }

    if(sim_flag == 0){
      // Update coefficients every specified number of blocks
      if(block_count%n_calc_blks == 0){

/*
        printf("CBF: synctime: %lu\n", synctime);
        printf("CBF: hclocks: %lu\n", hclocks);
        printf("CBF: fenchan: %lu\n", (unsigned long)fenchan);
        printf("CBF: chan_bw: %lf\n", chan_bw);
        printf("CBF: pktidx: %ld\n", pktidx);
        printf("CBF: obsfreq (MHz): %lf\n", obsfreq);
*/

        // Calc real-time seconds since SYNCTIME for pktidx:
        //
        //                        pktidx * hclocks
        //     realtime_secs = -----------------------
        //                      2e6 * fenchan * chan_bw
        // This is the value that will be used in the delay polynomial (t, the epoch)
        if(fenchan * chan_bw != 0.0) {
          realtime_secs = (pktidx * hclocks) / (2e6 * fenchan * fabs(chan_bw));
        }

        epoch_sec = synctime + realtime_secs;

        printf("CBF: Before open(myfifo_e), epoch_sec = %lf...\n", epoch_sec);
  
        // Write to new FIFO going to get_delays module for delay polynomial calculation
        // Creating the named file(FIFO)
        // mkfifo(<pathname>,<permission>)
        mkfifo(myfifo_e, 0666);

        while(access(myfifo_e, F_OK) == 0){
          // If the FIFO has been deleted/removed by the get_delays module,
          // then that means it has been read and we can go retrieve the delay polynomials
          if(access(myfifo_e, F_OK) != 0){
            printf("CBF: FIFO used to write epoch for get_delays script was deleted...\n");
            break;
          }

          // Open file as a read/write
          fde = open(myfifo_e,O_RDWR);
          if(fde == -1){
            printf("CBF: Unable to open fifo_e file...\n");
            break;
          }

          //printf("CBF: After open(myfifo_e)...\n");

          // Write to file
          wvale = write(fde, &epoch_sec, sizeof(epoch_sec));
          if(wvale != 0){
            printf("CBF: written to epoch FIFO!\n");
          }

          //printf("CBF: After write(myfifo_e)...\n");

          // Close file
          close(fde);
        }

        // Wait for this FIFO to be created by get_delays module
        while(access(myfifo, F_OK) != 0){
          if(access(myfifo, F_OK) == 0){
            printf("CBF: FIFO for getting delay polynomials was created...\n");
            break;
          }
        }

        // Now get the newly calculated polynomials from FIFO written to by get_delays module
        /* Periodically get delay polynomials */
        // First check to see whether the file in /tmp used as a FIFO exists, currently called katpoint_delays and wait until it's deleted
        // If it exists, read the delays from the FIFO then compute new beamformer coefficients -> while( access( fname, F_OK ) == 0 )
        // If it doesn't exist then that means no new delays have been calculated so continue on with the previously calculated delays.
        while(access(myfifo, F_OK) == 0){
          // If the FIFO has been deleted/removed by the get_delays module,
          // then that means it has been read and we can now generate the coefficients
          if(access(myfifo, F_OK) != 0){
            printf("CBF: FIFO for getting delay polynomials was deleted...\n");
            break;
          }

          // Open file as a read only
          //printf("CBF: Before open(myfifo)...\n");
          fdr = open(myfifo,O_RDWR);
          if(fdr == -1){
            printf("CBF: Unable to open fifo file...\n");
            break;
          }

          //printf("CBF: After open(myfifo)...\n");

          // Read file
          read_val = read(fdr, delay_pols, sizeof(delay_pols));
          if(read_val != 0){
            printf("CBF: Read delays \n");
          }

          // Close file
          close(fdr);
        }
      }

      // Update coefficients every specified number of blocks
      if(block_count%n_update_blks == 0){

        // If statement to make sure the epoch_sec calculation isn't done twice unnecessarily
        if(block_count%n_calc_blks != 0){
          // Calc real-time seconds since SYNCTIME for pktidx:
          //
          //                        pktidx * hclocks
          //     realtime_secs = -----------------------
          //                      2e6 * fenchan * chan_bw
          // This is the value that will be used in the delay polynomial (t, the epoch)
          if(fenchan * chan_bw != 0.0) {
            realtime_secs = (pktidx * hclocks) / (2e6 * fenchan * fabs(chan_bw));
          }

          epoch_sec = synctime + realtime_secs;
        }

        printf("CBF: In update coefficients if(), epoch_sec = %lf...\n", epoch_sec);

        // Update coefficients with realtime_secs
        // Assign values to tmp variable then copy values from it to pinned memory pointer (bf_coefficients)
        tmp_coefficients = generate_coefficients(delay_pols, coarse_chan_freq, n_chan_per_node, nants, epoch_sec);
        //tmp_coefficients = generate_coefficients(delay_pols, telstate_phase, coarse_chan_freq, n_chan_per_node, nants, epoch_sec);
        memcpy(bf_coefficients, tmp_coefficients, N_COEFF*sizeof(float));
      }

    }

    // Get the appropriate basefile name from the rawfile_input_thread  
    if(filenum == 0 && block_count == 0){ // Check for the basefile name at the beginning of the first file in the 5 minute period
      hashpipe_status_lock_safe(st);
      hgets(st->buf, "BASEFILE", sizeof(raw_basefilename), raw_basefilename);
      hgets(st->buf, "OUTDIR", sizeof(outdir), outdir);
      hashpipe_status_unlock_safe(st);
      printf("CBF: RAW file base filename from command: %s and outdir is: %s \n", raw_basefilename, outdir);

      // Get filterbank file path
      // strrchr() finds the last occurence of the specified character
      char_offset = strrchr(raw_basefilename, character);
      slash_pos = char_offset-raw_basefilename;
      printf("The last position of %c is %ld \n", character, slash_pos);

      // Get file name with no path
      memcpy(new_base, &raw_basefilename[slash_pos+1], sizeof(raw_basefilename)-slash_pos);
      printf("File name with no path: %s \n", new_base);

      // Set specified path to write filterbank files
      strcpy(basefilename, outdir);
      strcat(basefilename, "/");
      strcat(basefilename, new_base);
      printf("File name with new path: %s \n", basefilename);
    }

    // If packet idx is NOT within start/stop range
    if(pktidx < pktstart || pktstop <= pktidx) {
      printf("CBF: Before checking whether files are open \n");
      for(int b = 0; b < N_BEAM; b++){
	// If file open, close it
	if(fdraw[b] != -1) {
	  // Close file
	  close(fdraw[b]);
	  // Reset fdraw, got_packet_0, filenum, block_count
	  fdraw[b] = -1;
	  if(b == 0){ // These variables only need to be set to zero once
	    got_packet_0 = 0;
	    //filenum = 0;
	    block_count=0;
	    // Print end of recording conditions
	    hashpipe_info(thread_name, "recording stopped: "
	      "pktstart %lu pktstop %lu pktidx %lu",
	      pktstart, pktstop, pktidx);
	  }
	}
      }
      printf("CBF: Before marking as free \n");
      /* Mark as free */
      hpguppi_input_databuf_set_free(db, curblock);

      /* Go to next block */
      curblock = (curblock + 1) % db->header.n_block;

      continue;
    }

    /* Set up data ptr for quant routines */
    pf.sub.data = (unsigned char *)hpguppi_databuf_data(db, curblock);

    // Wait for packet 0 before starting write
    // "packet 0" is the first packet/block of the new recording,
    // it is not necessarily pktidx == 0.
    if (got_packet_0==0 && gp.stt_valid==1) {
      got_packet_0 = 1;

      char fname[256];
      // Create the output directory if needed
      char datadir[1024];
      strncpy(datadir, basefilename, 1023);
      char *last_slash = strrchr(datadir, '/');
      if (last_slash!=NULL && last_slash!=datadir) {
	*last_slash = '\0';
	hashpipe_info(thread_name,
	  "Using directory '%s' for output", datadir);
        if(mkdir_p(datadir, 0755) == -1) {
	  hashpipe_error(thread_name, "mkdir_p(%s)", datadir);
          pthread_exit(NULL);
        }
      }

      // Update filterbank headers based on raw params and Nts etc.
      // Possibly here
      printf("CBF: update_fb_hdrs_from_raw_hdr_cbf(fb_hdr, ptr) \n");
      update_fb_hdrs_from_raw_hdr_cbf(fb_hdr, ptr);

      hgetr8(ptr, "OBSBW", &obsbw);
      hgetr8(ptr,"TBIN", &tbin);
      

      // Open N_BEAM filterbank files to save a beam per file i.e. N_BIN*nt*sizeof(float) per file.
      printf("CBF: Opening filterbank files \n");
      for(int b = 0; b < N_BEAM; b++){
        if(b >= 0 && b < 10) {
	  sprintf(fname, "%s.%04d-cbf0%d.fil", basefilename, filenum, b);
        }else{
          sprintf(fname, "%s.%04d-cbf%d.fil", basefilename, filenum, b);
        }
        hashpipe_info(thread_name, "Opening fil file '%s'", fname);
	  last_slash = strrchr(fname, '/');
        if(last_slash) {
	  strncpy(fb_hdr.rawdatafile, last_slash+1, 80);
        } else {
	  strncpy(fb_hdr.rawdatafile, fname, 80);
        }
        fb_hdr.rawdatafile[80] = '\0';
                
                
        fdraw[b] = open(fname, O_CREAT|O_WRONLY|O_TRUNC, 0644);
        if(fdraw[b] == -1) {
	  // If we can't open this output file, we probably won't be able to
          // open any more output files, so print message and bail out.
          hashpipe_error(thread_name,
	    "cannot open filterbank output file, giving up");
            pthread_exit(NULL);
        }
    	posix_fadvise(fdraw[b], 0, 0, POSIX_FADV_DONTNEED);
      }
      printf("CBF: Opened filterbank files after for() loop \n");

      //Fix some header stuff here due to multi-antennas
      // Need to understand and appropriately modify these values if necessary
      // Default values for now
      fb_hdr.foff = obsbw;
      fb_hdr.nchans = n_chan_per_node;
      fb_hdr.fch1 = obsfreq;
      fb_hdr.nbeams = N_BEAM;
      fb_hdr.tsamp = tbin * nt;
    }

    /* See if we need to open next file */
    if (block_count >= MAX_BLKS_PER_FILE) {
      filenum++;
      if(filenum >= N_FILE){
        filenum = 0;
      }else{
        char fname[256];
        for(int b = 0; b < N_BEAM; b++){
          close(fdraw[b]);
          if(b >= 0 && b < 10) {
            sprintf(fname, "%s.%04d-cbf0%d.fil", basefilename, filenum, b);
          }else{
            sprintf(fname, "%s.%04d-cbf%d.fil", basefilename, filenum, b);
          }
          open_flags = O_CREAT|O_RDWR|O_SYNC;
          fprintf(stderr, "CBF: Opening next fil file '%s'\n", fname);
          fdraw[b] = open(fname, open_flags, 0644);
          if (fdraw[b]==-1) {
            hashpipe_error(thread_name, "Error opening file.");
            pthread_exit(NULL);
          }
        }
      }
      block_count=0;
    }

    /* If we got packet 0, process and write data to disk */
    if (got_packet_0) {

      /* Note writing status */
      hashpipe_status_lock_safe(st);
      hputs(st->buf, status_key, "writing");
      hashpipe_status_unlock_safe(st);

      /* Write filterbank header to output file */
      printf("CBF: fb_fd_write_header(fdraw[b], &fb_hdr); \n");
      if(block_count == 0){
        printf("CBF: Writing headers to filterbank files! \n");
	for(int b = 0; b < N_BEAM; b++){
	  fb_hdr.ibeam =  b;
	  fb_fd_write_header(fdraw[b], &fb_hdr);
	}
      }

      printf("CBF: Before run_beamformer! \n");
      /* Write data */
      // gpu processing function here, I think...

      // Start timing beamforming computation
      struct timespec tval_before, tval_after;
      clock_gettime(CLOCK_MONOTONIC, &tval_before);

      if(sim_flag == 1){
        output_data = run_beamformer((signed char *)&db->block[curblock].data, tmp_coefficients, n_chan_per_node, nt);
      }
      if(sim_flag == 0){
        output_data = run_beamformer((signed char *)&db->block[curblock].data, bf_coefficients, n_chan_per_node, nt);
      }

      /* Set beamformer output (CUDA kernel before conversion to power), that is summing, to zero before moving on to next block*/
      set_to_zero();

      // Stop timing beamforming computation
      clock_gettime(CLOCK_MONOTONIC, &tval_after);
      time_taken = (float)(tval_after.tv_sec - tval_before.tv_sec); //*1e6; // Time in seconds since epoch
      time_taken = time_taken + (float)(tval_after.tv_nsec - tval_before.tv_nsec)*1e-9; // Time in nanoseconds since 'tv_sec - start and end'
      bf_time = time_taken;

      printf("CBF: run_beamformer() plus set_to_zero() time: %f s\n", bf_time);

      printf("CBF: First element of output data: %f and writing to filterbank files \n", output_data[0]);

      // Start timing write
      struct timespec tval_before_w, tval_after_w;
      clock_gettime(CLOCK_MONOTONIC, &tval_before_w);

      // This may be okay to write to filterbank files, but I'm not entirely confident
      for(int b = 0; b < N_BEAM; b++){
	//rv = write(fdraw[b], &output_data[b*nt*n_chan_per_node], (size_t)blocksize);
        ns = nt/N_TIME_STI;
        rv = write(fdraw[b], &output_data[b*ns*n_chan_per_node], (size_t)blocksize);
	if(rv != blocksize){
	  char msg[100];
          perror(thread_name);
	  sprintf(msg, "Error writing data (output_data=%p, blocksize=%d, rv=%d)", output_data, blocksize, rv);
          hashpipe_error(thread_name, msg);
        }

	/* flush output */
	fsync(fdraw[b]);
      }

      // Stop timing write
      clock_gettime(CLOCK_MONOTONIC, &tval_after_w);
      time_taken_w = (float)(tval_after_w.tv_sec - tval_before_w.tv_sec); //*1e6; // Time in seconds since epoch
      time_taken_w = time_taken_w + (float)(tval_after_w.tv_nsec - tval_before_w.tv_nsec)*1e-9; // Time in nanoseconds since 'tv_sec - start and end'
      write_time = time_taken_w;
      printf("CBF: Time taken to write block of size, %d bytes, to disk = %f s \n", N_BEAM*blocksize, write_time);
      
      printf("CBF: After write() function! Block index = %d \n", block_count);

      /* Increment counter */
      block_count++;
    }

    /* Mark as free */
    hpguppi_input_databuf_set_free(db, curblock);

    /* Go to next block */
    curblock = (curblock + 1) % db->header.n_block;

    /* Check for cancel */
    pthread_testcancel();

  }

  pthread_cleanup_pop(0); // Closes safe_close 

  pthread_cleanup_pop(0); /* Closes hpguppi_free_psrfits */

  // Free up device memory
  cohbfCleanup();

  hashpipe_info(thread_name, "exiting!");
  pthread_exit(NULL);
}

static hashpipe_thread_desc_t rawdisk_thread = {
  name: "hpguppi_coherent_bf_thread",
  skey: "DISKSTAT",
  init: NULL,
  run:  run,
  ibuf_desc: {hpguppi_input_databuf_create},
  obuf_desc: {NULL}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&rawdisk_thread);
}

// vi: set ts=8 sw=2 noet :
