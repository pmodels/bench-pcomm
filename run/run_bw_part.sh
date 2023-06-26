#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

#!/bin/bash

# run bandwidth test for partitioned communications
# arguments:
# - mpi_version the flavor of 'ompi' or 'mpich'
# - mpi_install_dir
# - n_threads
# - optional list of env variables to set
# 
function run_bw_part(){
    mpi_version=${1}
    mpi_install_dir=${2}
    n_threads=${3}

    # remove the first three items on the list (https://unix.stackexchange.com/questions/68322/how-can-i-remove-an-element-from-an-array-completely)
	for env in "${@:4}"
	do
	    export ${env}
	    echo "${env}" >> env.info
	done

    echo "=================================================================="
    echo "compilation..."
    echo "mpi_install_dir = ${mpi_install_dir}"
    make clean
    MPI_DIR=${mpi_install_dir} ARCH_FILE=make_arch/lucia.mak make -j 8
    echo "=================================================================="
    #-----------------------------------------------------------------------
    if [[ "${mpi_version}" == "mpich" ]]
    then
        echo "MPIR_CVAR_DEBUG_SUMMARY       = ${MPIR_CVAR_DEBUG_SUMMARY}"
        echo "MPIR_CVAR_CH4_NUM_VCIS        = ${MPIR_CVAR_CH4_NUM_VCIS}"
        echo "MPIR_CVAR_CH4_ASYNC_PROGRESS  = ${MPIR_CVAR_CH4_ASYNC_PROGRESS}"
        echo "MPIR_CVAR_CH4_GLOBAL_PROGRESS = ${MPIR_CVAR_CH4_GLOBAL_PROGRESS}"
        echo "MPIR_CVAR_PART_AGGR_SIZE      = ${MPIR_CVAR_PART_AGGR_SIZE}"
        echo "MPIR_CVAR_CH4_RESERVE_VCIS    = ${MPIR_CVAR_CH4_RESERVE_VCIS}"
        ${mpi_install_dir}/bin/mpichversion >> env.info
        MPI_CONF="-n 2 -bind-to core:${n_threads} -ppn 1 -l"

        # generate hostfile for ompi
        #${mpi_install_dir}/bin/mpiexec ${MPI_CONF} hostname > ${HOME}/temp_hostfile_2_${n_threads}.txt
    fi
    #-----------------------------------------------------------------------
    # OMPI CONFIGURATION
    if [[ "${mpi_version}" == "ompi" ]]
    then
        nodelist=$(scontrol show hostname $SLURM_NODELIST)
        printf "%s\n" "${nodelist[@]}" > nodefile
        MPI_CONF="-np 2 --map-by ppr:1:node:pe=${n_threads} --report-bindings"
        #MPI_CONF="-np 2 --map-by ppr:1:node:pe=${n_threads} --report-bindings --mca pml ^ucx"
    fi
    if [[ "${mpi_version}" == "ompi-nobind" ]]
    then
        nodelist=$(scontrol show hostname $SLURM_NODELIST)
        printf "%s\n" "${nodelist[@]}" > nodefile
        MPI_CONF="-np 2 --npernode 1 --report-bindings"
        #MPI_CONF="-np 2 --map-by ppr:1:node:pe=${n_threads} --report-bindings --mca pml ^ucx"
    fi

    #-----------------------------------------------------------------------
    # OPENMP configuration
    export OMP_PROC_BIND=close
    export OMP_PLACES=cores
    export OMP_NUM_THREADS=${n_threads}
    echo "OMP_NUM_THREADS=${OMP_NUM_THREADS}"

    #-----------------------------------------------------------------------
    echo "=================================================================="
    echo " mpi cmd: ${OMP_CONF} ${mpi_install_dir}/bin/mpiexec ${MPI_CONF}"
    echo "------------------------------------------------------------------"
    ${mpi_install_dir}/bin/mpiexec ${MPI_CONF} ./benchme
    echo "done...."
    echo "------------------------------------------------------------------"

}
# end of file
