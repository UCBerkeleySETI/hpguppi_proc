#!/bin/bash
#
# readraw_init.sh - Read GUPPI RAW files
# Usage: readraw_init.sh [mode] [input] [output]
#
#     mode   - readonly|cp|fil|cbf|ubf
#     input  - input dir
#     output - (optional)
#
# If mode is cbf or ubf, then no input and output are necessary
# Input and output will be provided by the status buffer


hpguppi_plugin=../src/.libs/hpguppi_proc.so

#Supported modes
if [ "$1" = 'readonly' ]
then
    net_thread=hpguppi_rawfile_input_thread
    out_thread=null_output_thread
    if [ -z "$2" ]
    then
        echo Input dir not provided.
        echo Usage: readraw_init.sh readonly [input]
        echo Exiting...
        exit
    else
        # Path to file in first argument of execution
        path_to_raw_files=$2

        # Finds the first RAW file in the directory (which has file number before the file extension "0000.raw")
        first_file=$(find $path_to_raw_files -type f -iname "*0000.raw")

        # Removes everything after the first period in the RAW file name
        basefile=${first_file%%.*}
        #basefile=$2
        outdir=${basefile}
        shift
    fi
elif [ "$1" = 'cp' ] || [ "$1" = 'fil' ] || [ "$1" = 'cbf' ] || [ "$1" = 'ubf' ]
then
    if [ "$1" = 'cp' ]
    then
        net_thread=hpguppi_rawfile_input_thread
        out_thread=hpguppi_rawdisk_only_thread
    elif [ "$1" = 'fil' ]
    then
        net_thread=hpguppi_rawfile_input_thread
        out_thread=hpguppi_fildisk_meerkat_thread
    elif [ "$1" = 'cbf' ]
    then
        net_thread=hpguppi_rawfile_input_thread
        out_thread=hpguppi_coherent_bf_thread
    else
        net_thread=hpguppi_stride_input_thread
	    out_thread=hpguppi_upchan_bf_thread
    fi

    if [ "$1" = 'cp' ] || [ "$1" = 'fil' ]
    then
        if [ -z "$2" ] || [ -z "$3" ]
        then
            echo Input/Output dir not provided.
            echo Usage: readraw_init.sh [mode] [input] [output]
            echo Exiting...
            exit
        else
            # Path to file in first argument of execution    
            path_to_raw_files=$2

            # Finds the first RAW file in the directory (which has file number before the file extension "0000.raw")
            first_file=$(find $path_to_raw_files -type f -iname "*0000.raw")

            # Removes everything after the first period in the RAW file name
            basefile=${first_file%%.*}
            #basefile=$2
            outdir=$3
            shift
        fi
    fi
else
    echo 'Unsupported mode. Choose between "readonly", "cp", "fil", "cbf", "ubf".'
    echo Exiting...
    exit
fi

#-------------------------------------------
if [ "$1" = 'cbf' ] || [ "$1" = 'ubf' ]
then
    if [ $# -eq 2 ]
    then
        inputdir=$2
        echo "Run Command:" hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir}

        hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir}
    elif [ $# -eq 3 ]
    then
        inputdir=$2
        bfrdir=$3
        echo "Run Command:" hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir} \
         -o BFRDIR=${bfrdir} 

        hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir} \
         -o BFRDIR=${bfrdir}
    elif [ $# -eq 4 ]
    then
        inputdir=$2
        bfrdir=$3
        outdir=$4
        echo "Run Command:" hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir} \
         -o BFRDIR=${bfrdir} \
         -o OUTDIR=${outdir}

        hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir} \
         -o BFRDIR=${bfrdir} \
         -o OUTDIR=${outdir}
    elif [ $# -eq 5 ]
    then
        inputdir=$2
        bfrdir=$3
        outdir=$4
        rawfile=$5
        echo "Run Command:" hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir} \
         -o BFRDIR=${bfrdir} \
         -o OUTDIR=${outdir} \
         -o RAWFILE=${rawfile}

        hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread \
         -o INPUTDIR=${inputdir} \
         -o BFRDIR=${bfrdir} \
         -o OUTDIR=${outdir} \
         -o RAWFILE=${rawfile}
    else
        echo "Run Command:" hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread

        hashpipe -p ${hpguppi_plugin:-hpguppi_proc} $net_thread $out_thread
    fi
else
    echo "No mode chosen."
    echo "Exiting ..."
    exit
fi
