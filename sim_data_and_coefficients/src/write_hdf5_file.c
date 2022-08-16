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
#define NCHAN_BFR5 64 // 16 or 64 or 512 for MK and 32 for COSMIC
#define NTIMES_BFR5 30
#define NBEAMS_BFR5 64
#define cal_all_idx(a, p, f, Na, Np) (a + Na * p + Np * Na * f)
#define delay_rates_idx(a, b, t, Na, Nb) (a + Na * b + Nb * Na * t)

#define FILE "/home/mruzinda/hpguppi_proc/sim_data_and_coefficients/test_data/guppi_bfr5_test_srcname_JBLAH-BLAH_NUM.bfr5"

int main()
{

    hid_t file, obsinfo, obsid_dataset, obsid_dataspace, obsid_datatype, calinfo, cal_all_dataset, cal_all_dataspace, cal_all_datatype; /* identifier */
    hid_t beaminfo, ra_dataset, ra_dataspace, ra_datatype;
    hid_t dec_dataset, dec_dataspace, dec_datatype;
    hid_t src_dataset, src_dataspace, native_src_type, src_datatype;
    hid_t delayinfo, delays_dataset, delays_dataspace;
    hid_t time_array_dataset, time_array_dataspace;
    hsize_t cal_all_dims[3];    /* cal_all dataset dimensions */
    hsize_t ra_dims;            /* ra dataset dimensions */
    hsize_t dec_dims;           /* dec dataset dimensions */
    hsize_t src_dims;           /* src name dataset dimensions */
    hsize_t obsid_dims;           /* obsid dataset dimensions */
    hsize_t delays_dims[3];     /* delays dataset dimensions */
    hsize_t time_array_dims; /* time_array dataset dimensions */
    herr_t status;
    int delays_elements = NTIMES_BFR5 * NBEAMS_BFR5 * NANTS_BFR5;

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
    uint64_t npol;
    hvl_t *src_names_str;

    // Allocate memory for array
    // cal_all_data = malloc((int)cal_all_elements * sizeof(complex_t));
    //delays_data = malloc(delays_elements * sizeof(double));
    time_array_data = malloc(NTIMES_BFR5 * sizeof(double));
    ra_data = malloc(NBEAMS_BFR5 * sizeof(double));
    dec_data = malloc(NBEAMS_BFR5 * sizeof(double));
    src_names_str = malloc(NBEAMS_BFR5 * sizeof(hvl_t));

    char b_str[256] = {0};
    char src_name_tmp[256];
    for (int i = 0; i < NBEAMS_BFR5; i++)
    {
        sprintf(b_str,"%02d",i);
        strcpy(src_name_tmp, "JBLAH-BLAH.B");
        strcat(src_name_tmp, b_str);
        src_names_str[i].p = src_name_tmp;
        src_names_str[i].len = strlen(src_names_str[i].p);
        printf("%d: len: %d, str is: %s\n", i, (int)src_names_str[i].len, (char *)src_names_str[i].p);
    }

    char obsid[256];
    strcpy(obsid, "MK-obsid");

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
    status = H5Dwrite(obsid_dataset, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, obsid);
    
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
