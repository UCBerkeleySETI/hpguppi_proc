#include <stdio.h>
#include <stdlib.h>
//#include <complex.h>
#include <math.h>


#define N_POL (2) //2                     // Number of polarizations
#define N_TIME (1024) // 8192 //16384 //1 // 8                   // Number of time samples
#define N_TIME_STI (8)
#define N_STI (N_TIME/N_TIME_STI)
#define N_STI_BLOC (N_TIME_STI)
#define N_STREAMS (1)                     // Number of CUDA streams
//#define N_COARSE_FREQ 64 //32               // Number of coarse channels processed at a time
#define MAX_COARSE_FREQ (512)                 // Max number of coarse channels is the number of channels in 32k mode
#define N_FINE_FREQ (1) //16384               // Number of fine channels per coarse channel 2^14 = 16384
//#define N_FREQ (N_COARSE_FREQ*N_FINE_FREQ) // Number of frequency bins after second FFT.  Should actually be 2^14, but due to limited memory on my laptop, arbitrarily 10
#define N_FREQ (MAX_COARSE_FREQ*N_FINE_FREQ) // Number of frequency bins after second FFT.  Should actually be 2^14, but due to limited memory on my laptop, arbitrarily 10
//#define N_FREQ_STREAM (N_FREQ/N_STREAMS) // (N_COARSE_FREQ*N_FINE_FREQ)/N_STREAMS // Number of frequency bins after second FFT.  Should actually be 2^14, but due to limited memory on my laptop, arbitrarily 10
//#define N_FREQ_STREAM (N_COARSE_FREQ/N_STREAMS) // (N_COARSE_FREQ*N_FINE_FREQ)/N_STREAMS // Number of frequency bins after second FFT.  Should actually be 2^14, but due to limited memory on my laptop, arbitrarily 10
#define N_ANT (64) // 64                  // Number of possible antennas (64 is also computationally efficient since it is a multiple of 32 which is the size of a warp)
#define N_REAL_ANT (61)                   // Number of antennas transmitting data downstream
#define N_BEAM (64) // 64                 // Number of beams

// "2" for inphase and quadrature
#define N_INPUT       (unsigned long int)(2*N_POL*N_TIME*N_FREQ*N_ANT)       // Size of input. Currently, same size as output
#define N_REAL_INPUT  (unsigned long int)(2*N_POL*N_TIME*N_FREQ*N_REAL_ANT)  // Size of input. Currently, same size as output
#define N_COEFF       (unsigned long int)(2*N_POL*N_ANT*N_BEAM*N_FREQ)       // Size of beamformer coefficients
//#define N_PHASE       (unsigned long int)(2*N_POL*N_ANT)                     // Size of telstate phase solution array - Include frequency dimension from meta data
#define DELAY_POLYS   (unsigned long int)(2)                                 // Number of coefficients in polynomial
#define N_DELAYS      (unsigned long int)(DELAY_POLYS*N_ANT*N_BEAM)          // Size of first order polynomial delay array
#define N_OUTPUT      (unsigned long int)(2*N_POL*N_BEAM*N_FREQ*N_TIME)      // Size of beamformer output
#define N_BF_POW      (unsigned long int)(N_BEAM*N_FREQ*N_TIME)              // Size of beamformer output after abs()^2
//#define N_BF_POW      (unsigned long int)(N_BEAM*N_FREQ*N_STI)               // Size of beamformer output after abs()^2 and short time integration

// VLASS specs
#define VLASS_N_TIME (10240000) // (5120000) // 1024000 with desired specs (~10 seconds) and 5120000 with required specs (~5 seconds)
#define VLASS_N_FREQ (1) // 1 of 32 coarse channels
#define VLASS_N_FFT  (128000) // Lowest frequency resolution
#define VLASS_N_BEAM (32) // Max number of beams
#define VLASS_N_WIN  (VLASS_N_TIME/VLASS_N_FFT) // Number of spectral windows

#define VLASS_N_INPUT       (unsigned long int)(2*N_POL*VLASS_N_TIME*VLASS_N_FREQ*(N_ANT/2))              // Size of input. Currently, same size as output
#define VLASS_N_COEFF       (unsigned long int)(2*N_POL*(N_ANT/2)*VLASS_N_BEAM*VLASS_N_FREQ)              // Size of beamformer coefficients
#define VLASS_N_BF_POW      (unsigned long int)((VLASS_N_BEAM+1)*VLASS_N_FREQ*VLASS_N_FFT*VLASS_N_WIN)        // Size of beamformer output after abs()^2 and short time integration


#ifndef min
#define min(a,b) ((a < b) ? a : b)
#endif
#ifndef max
#define max(a,b) ((a > b) ? a : b)
#endif

#define PI (3.14159265)

// p - polarization index
// t - time index
// f - frequency index
// a - antenna index
// b - beam index
#define data_in_idx(p, t, f, a, Np, Nt, Nf)     ((p) + (Np)*(t) + (Nt)*(Np)*(f) + (Nf)*(Nt)*(Np)*(a))
// Don't need an "N_REAL_INPUT" macro since the antennas are initially the slowest moving index 
#define data_tr_idx(a, p, f, t, Np, Nf)         ((a) + (N_ANT)*(p) + (Np)*(N_ANT)*(f) + (Nf)*(Np)*(N_ANT)*(t))
#define coeff_idx(a, p, b, f, Np, Nb)           ((a) + (N_ANT)*(p) + (Np)*(N_ANT)*(b) + (Nb)*(Np)*(N_ANT)*(f))
//#define phase_idx(a, p, f, Np)                  ((a) + (N_ANT)*(p) + (Np)*(N_ANT)*(f))
//#define delay_idx(d, a, b, Na)                  ((d) + DELAY_POLYS*(a) + DELAY_POLYS*(Na)*(b)) // Should be correct indexing
#define cal_all_idx(a, p, f, Na, Np)            ((a) + (Na)*(p) + (Np)*(Na)*(f))
#define delay_idx(a, b, t, Na, Nb)              ((a) + (Na)*(b) + (Nb)*(Na)*(t))
#define coh_bf_idx(p, b, f, t, Np, Nb, Nf)      ((p) + (Np)*(b) + (Nb)*(Np)*(f) + (Nf)*(Nb)*(Np)*(t))
#define pow_bf_nosti_idx(b, f, t, Nf, Nt)       ((f) + (Nf)*(t) + (Nf)*(Nt)*(b)) // Changed to efficiently write each beam to a filterbank file
#define pow_bf_idx(f, s, b, Nf, Ns)             ((f) + (Nf)*(s) + (Nf)*(Ns)*(b)) // Changed to efficiently write each beam to a filterbank file
//#define pow_bf_idx(b, f, s, Nf, Ns)             ((f) + (Nf)*(s) + (Nf)*(Ns)*(b)) // Changed to efficiently write each beam to a filterbank file

typedef struct complex_t{
	float re;
	float im;
}complex_t;

#ifdef __cplusplus
extern "C" {
#endif
void init_beamformer(); // Allocate memory to all arrays 
void set_to_zero(); // Set arrays to zero after a block is processed
signed char* simulate_data(int n_pol, int n_chan, int n_samp);
float* simulate_coefficients(int n_pol, int n_beam, int n_chan);
float* generate_coefficients(complex_t* phase_up, double* delay, int n, double* coarse_chan, int n_pol, int n_beam, int schan, int n_chan, uint64_t n_real_ant);
void input_data_pin(signed char * data_in_pin);
void output_data_pin(float * data_out_pin);
void coeff_pin(float * data_coeff_pin);
void unregister_data(void * data_unregister);
void cohbfCleanup();
float* run_beamformer(signed char* data_in, float* coefficient, int n_pol, int n_beam, int n_chan, int n_samp); // Run beamformer
#ifdef __cplusplus
}
#endif
