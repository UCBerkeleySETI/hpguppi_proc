ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS     =

AM_CPPFLAGS += -I"@HASHPIPE_INCDIR@"
AM_CPPFLAGS += -I"@CBF_INCDIR@"
AM_CPPFLAGS += -I"@UBF_INCDIR@"

# AM_CFLAGS is used for all C compiles
AM_CFLAGS = -ggdb -fPIC -O3 -Wall -Werror -Wno-error=cpp -fno-strict-aliasing @X86_FEATURE_CFLAGS@

#AM_CFLAGS += -Wno-error=stringop-truncation -Wno-error=format-overflow -Wno-error=unused-const-variable -Wno-error=sizeof-pointer-div
AM_CFLAGS += -I"/usr/include/hdf5/serial"
AM_CFLAGS += -L"/lib/x86_64-linux-gnu/hdf5/serial"

AM_CFLAGS += -I"@BFR5C99_INCDIR@" -I"@RAWSPEC_INCDIR@"

# Convenience variables to group source files
hpguppi_databuf = hpguppi_databuf.h \
		  hpguppi_databuf.c

hpguppi_support = hpguppi_rawspec.h \
                  hpguppi_rawspec.c \
                  hpguppi_params.c \
		  		  hpguppi_time.h   \
		  		  hpguppi_time.c   \
		  		  hpguppi_util.h   \
		  		  hpguppi_util.c   \
		  	   	  psrfits.h

libsla_support = slalib.h sla.c f77.h

hpguppi_threads = hpguppi_rawdisk_only_thread.c \
		  		  hpguppi_fildisk_meerkat_thread.c \
		  		  hpguppi_rawfile_input_thread.c \
		  		  hpguppi_coherent_bf_thread.c \
				  hpguppi_stride_input_thread.c \
		  		  hpguppi_upchan_bf_thread.c

# This is the hpguppi_proc plugin
lib_LTLIBRARIES = hpguppi_proc.la
hpguppi_proc_la_SOURCES  = $(hpguppi_databuf) $(hpguppi_support) $(libsla_support) $(hpguppi_threads)
#hpguppi_proc_la_LIBADD   = -lsla -lrt -lrawspec -lhashpipe_ibverbs -lcoherent_beamformer -lm -L/opt/conda/lib -lpython3.7m
#hpguppi_proc_la_LIBADD   = -lsla -lrt -lrawspec -lhashpipe_ibverbs -lcoherent_beamformer -lm -lpython3.7m
hpguppi_proc_la_LIBADD   = -lsla -lrt -lrawspec -lcoherent_beamformer -lupchannelizer_beamformer -lcufft -L/usr/local/cuda/lib64 -lm -lbfr5c99
hpguppi_proc_la_LDFLAGS  = -avoid-version -module -shared -export-dynamic
hpguppi_proc_la_LDFLAGS += -L"@LIBSLADIR@" -Wl,-rpath,"@LIBSLADIR@"
hpguppi_proc_la_LDFLAGS += -L"@HASHPIPE_LIBDIR@" -Wl,-rpath,"@HASHPIPE_LIBDIR@"
hpguppi_proc_la_LDFLAGS += -L"@CBF_LIBDIR@" -Wl,-rpath,"@CBF_LIBDIR@"
hpguppi_proc_la_LDFLAGS += -L"@UBF_LIBDIR@" -Wl,-rpath,"@UBF_LIBDIR@"
hpguppi_proc_la_LDFLAGS += -L"@BFR5C99_LIBDIR@" -Wl,-rpath,"@BFR5C99_LIBDIR@"
hpguppi_proc_la_LDFLAGS += -L"@RAWSPEC_LIBDIR@" -Wl,-rpath,"@RAWSPEC_LIBDIR@"

# vi: set ts=8 noet :
