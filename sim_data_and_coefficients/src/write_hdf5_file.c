// This script creates and writes HDF5 files to test the upchannelizer beamformer thread
// To compile it:
// gcc write_hdf5_file.c -o write_hdf5_file -lm -lhdf5
// To run it:
// ./write_hdf5_file

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <endian.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "hdf5.h"

#define NANTS_BFR5 64
#define NPOL_BFR5 2
#define NCHAN_BFR5 4096 // 16 or 64 or 512 for MK and 32 for COSMIC
#define NTIMES_BFR5 30
#define NBEAMS_BFR5 64
#define cal_all_idx(a, p, f, Na, Np) (a + Na * p + Np * Na * f)
#define delay_rates_idx(a, b, t, Na, Nb) (a + Na * b + Nb * Na * t)

#define FILE "/home/mruzinda/hpguppi_proc/sim_data_and_coefficients/test_data/guppi_bfr5_test_srcname_JBLAH-BLAH_NUM.bfr5"
#define OBSID_STR "MK-obsid"
#define OBSID_LEN sizeof(OBSID_STR)

int main()
{

    hid_t file, obsinfo, obsid_dataset, obsid_dataspace, obsid_datatype, calinfo, cal_all_dataset, cal_all_dataspace, cal_all_datatype; /* identifier */
    hid_t beaminfo, ra_dataset, ra_dataspace, ra_datatype, diminfo, npol_dataspace, nbeams_dataspace, npol_dataset, nbeams_dataset;
    hid_t dec_dataset, dec_dataspace, dec_datatype;
    hid_t src_dataset, src_dataspace, native_src_type, src_datatype;
    hid_t delayinfo, delays_dataset, delays_dataspace, rates_dataset, rates_dataspace;
    hid_t time_array_dataset, time_array_dataspace;
    hsize_t cal_all_dims[3];    /* cal_all dataset dimensions */
    hsize_t ra_dims;            /* ra dataset dimensions */
    hsize_t dec_dims;           /* dec dataset dimensions */
    hsize_t delays_dims[3];     /* delays dataset dimensions */
    hsize_t rates_dims[3];     /* rates dataset dimensions */
    hsize_t time_array_dims; /* time_array dataset dimensions */
    herr_t status;
    int delays_elements = NTIMES_BFR5 * NBEAMS_BFR5 * NANTS_BFR5;
    int rates_elements = NTIMES_BFR5 * NBEAMS_BFR5 * NANTS_BFR5;

    // Create complex data type for call_all dataset
    typedef struct complex_t
    {
        float re;
        float im;
    } complex_t;

    hid_t reim_tid;
    reim_tid = H5Tcreate(H5T_COMPOUND, sizeof(complex_t));
    H5Tinsert(reim_tid, "r", HOFFSET(complex_t, re), H5T_IEEE_F32LE);
    H5Tinsert(reim_tid, "i", HOFFSET(complex_t, im), H5T_IEEE_F32LE);

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
    for (int i = 0; i < NTIMES_BFR5; i++)
    {
        for (int j = 0; j < NBEAMS_BFR5; j++)
        {
            for (int k = 0; k < NANTS_BFR5; k++)
            {
                delays_data[i][j][k] = 0;
            }
        }
    }
    double rates_data[NTIMES_BFR5][NBEAMS_BFR5][NANTS_BFR5];
    for (int i = 0; i < NTIMES_BFR5; i++)
    {
        for (int j = 0; j < NBEAMS_BFR5; j++)
        {
            for (int k = 0; k < NANTS_BFR5; k++)
            {
                rates_data[i][j][k] = 0;
            }
        }
    }
    double *time_array_data;
    double *ra_data;
    double *dec_data;
    uint64_t nbeams = 64;
    uint64_t npol = 2;
    char *src_names_str[NBEAMS_BFR5] = {"BORESIGHT.B00", "JBLAH-BLAH.B01", "JBLAH-BLAH.B02", "JBLAH-BLAH.B03",
    "JBLAH-BLAH.B04", "JBLAH-BLAH.B05", "JBLAH-BLAH.B06", "JBLAH-BLAH.B07", "JBLAH-BLAH.B08", "JBLAH-BLAH.B09", 
    "JBLAH-BLAH.B10", "JBLAH-BLAH.B11", "JBLAH-BLAH.B12", "JBLAH-BLAH.B13", "JBLAH-BLAH.B14", "JBLAH-BLAH.B15", 
    "JBLAH-BLAH.B16", "JBLAH-BLAH.B17", "JBLAH-BLAH.B18", "JBLAH-BLAH.B19", "JBLAH-BLAH.B20", "JBLAH-BLAH.B21", 
    "JBLAH-BLAH.B22", "JBLAH-BLAH.B23", "JBLAH-BLAH.B24", "JBLAH-BLAH.B25", "JBLAH-BLAH.B26", "JBLAH-BLAH.B27", 
    "JBLAH-BLAH.B28", "JBLAH-BLAH.B29", "JBLAH-BLAH.B30", "JBLAH-BLAH.B31", "JBLAH-BLAH.B32", "JBLAH-BLAH.B33", 
    "JBLAH-BLAH.B34", "JBLAH-BLAH.B35", "JBLAH-BLAH.B36", "JBLAH-BLAH.B37", "JBLAH-BLAH.B38", "JBLAH-BLAH.B39", 
    "JBLAH-BLAH.B40", "JBLAH-BLAH.B41", "JBLAH-BLAH.B42", "JBLAH-BLAH.B43", "JBLAH-BLAH.B44", "JBLAH-BLAH.B45", 
    "JBLAH-BLAH.B46", "JBLAH-BLAH.B47", "JBLAH-BLAH.B48", "JBLAH-BLAH.B49", "JBLAH-BLAH.B50", "JBLAH-BLAH.B51", 
    "JBLAH-BLAH.B52", "JBLAH-BLAH.B53", "JBLAH-BLAH.B54", "JBLAH-BLAH.B55", "JBLAH-BLAH.B56", "JBLAH-BLAH.B57", 
    "JBLAH-BLAH.B58", "JBLAH-BLAH.B59", "JBLAH-BLAH.B60", "JBLAH-BLAH.B61", "JBLAH-BLAH.B62", "JBLAH-BLAH.B63"};
    size_t src_size;
    size_t obs_size;

    // Allocate memory for array
    time_array_data = malloc(NTIMES_BFR5 * sizeof(double));
    ra_data = malloc(NBEAMS_BFR5 * sizeof(double));
    dec_data = malloc(NBEAMS_BFR5 * sizeof(double));

    char b_str[256] = {0};
    char src_name_tmp[256];
    int src_strlen = 0;

    char obsid[1][OBSID_LEN] = {OBSID_STR};

    int Nant = 63;   // Number of antennas
    int Nbeams = 61; // Number of beams
    int Ntimes = 30; // Number of time stamps
    int Npol = 2;    // Number of polarizations

    /*
     * Create a new file using H5ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     * Then close the file.
     */
    file = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create a group in the file.
     */
    calinfo = H5Gcreate(file, "/calinfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    beaminfo = H5Gcreate(file, "/beaminfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    delayinfo = H5Gcreate(file, "/delayinfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    obsinfo = H5Gcreate(file, "/obsinfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    diminfo = H5Gcreate(file, "/diminfo", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
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

    rates_dims[0] = NTIMES_BFR5;
    rates_dims[1] = NBEAMS_BFR5;
    rates_dims[2] = NANTS_BFR5;
    rates_dataspace = H5Screate_simple(3, rates_dims, NULL);

    time_array_dims = NTIMES_BFR5;
    time_array_dataspace = H5Screate_simple(1, &time_array_dims, NULL);

    ra_dims = NBEAMS_BFR5;
    ra_dataspace = H5Screate_simple(1, &ra_dims, NULL);

    dec_dims = NBEAMS_BFR5;
    dec_dataspace = H5Screate_simple(1, &dec_dims, NULL);

    printf("Here 1\n");
    hsize_t src_dims[1] = {NBEAMS_BFR5}; /* src name dataset dimensions */
    src_dataspace = H5Screate_simple(1, src_dims, NULL);

    printf("Here 2\n");

    hsize_t obsid_dims[1] = {1}; /* obsid dataset dimensions */
    obsid_dataspace = H5Screate_simple(1, obsid_dims, NULL);

    printf("Here 3\n");

    npol_dataspace = H5Screate(H5S_SCALAR);

    nbeams_dataspace = H5Screate(H5S_SCALAR);

    /*
     * Define datatype for the data in the file.
     * We will store little endian INT numbers.
     */
    cal_all_datatype = H5Tcopy(reim_tid);
    status = H5Tset_order(cal_all_datatype, H5T_ORDER_LE);

    src_datatype = H5Tcopy(H5T_C_S1);
    status = H5Tset_size(src_datatype, H5T_VARIABLE);

    obsid_datatype = H5Tcopy(H5T_C_S1);
    obs_size = (int)strlen(obsid[0])*sizeof(char);
    status = H5Tset_size(obsid_datatype, obs_size);

    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    cal_all_dataset = H5Dcreate(file, "/calinfo/cal_all", cal_all_datatype, cal_all_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    delays_dataset = H5Dcreate(file, "/delayinfo/delays", H5T_IEEE_F64LE, delays_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    rates_dataset = H5Dcreate(file, "/delayinfo/rates", H5T_IEEE_F64LE, rates_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    time_array_dataset = H5Dcreate(file, "/delayinfo/time_array", H5T_IEEE_F64LE, time_array_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    ra_dataset = H5Dcreate(file, "/beaminfo/ras", H5T_IEEE_F64LE, ra_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dec_dataset = H5Dcreate(file, "/beaminfo/decs", H5T_IEEE_F64LE, dec_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    src_dataset = H5Dcreate(file, "/beaminfo/src_names", src_datatype, src_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    obsid_dataset = H5Dcreate(file, "/obsinfo/obsid", obsid_datatype, obsid_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    npol_dataset = H5Dcreate(file, "/diminfo/npol", H5T_STD_I64LE, npol_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    nbeams_dataset = H5Dcreate(file, "/diminfo/nbeams", H5T_STD_I64LE, nbeams_dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    
    printf("Here 4\n");
    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(cal_all_dataset, reim_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, cal_all_data);
    printf("Here 5\n");
    status = H5Dwrite(delays_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, delays_data);
    printf("Here 6\n");
    status = H5Dwrite(rates_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rates_data);
    printf("Here 7\n");
    status = H5Dwrite(time_array_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, time_array_data);
    printf("Here 8\n");
    status = H5Dwrite(ra_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, ra_data);
    printf("Here 9\n");
    status = H5Dwrite(dec_dataset, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, dec_data);
    printf("Here 10\n");
    status = H5Dwrite(src_dataset, src_datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, src_names_str);
    printf("Here 11\n");
    status = H5Dwrite(obsid_dataset, obsid_datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, obsid);
    printf("Here 12\n");
    status = H5Dwrite(npol_dataset, H5T_STD_I64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &npol);
    printf("Here 13\n");
    status = H5Dwrite(nbeams_dataset, H5T_STD_I64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &nbeams);
    printf("Here 14\n");

    /*
     * Close/release resources
     */
    status = H5Sclose(cal_all_dataspace);
    status = H5Tclose(cal_all_datatype);
    status = H5Dclose(cal_all_dataset);

    status = H5Sclose(delays_dataspace);
    status = H5Dclose(delays_dataset);

    status = H5Sclose(rates_dataspace);
    status = H5Dclose(rates_dataset);

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

    status = H5Sclose(npol_dataspace);
    status = H5Dclose(npol_dataset);

    status = H5Sclose(nbeams_dataspace);
    status = H5Dclose(nbeams_dataset);

    status = H5Fclose(file);
    
    return 0;
}
