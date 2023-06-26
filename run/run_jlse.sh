#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

#!/bin/bash
#COBALT --time 2:00:00
#COBALT --nodecount 2
#COBALT --queue skylake_8180


#-------------------------------------------------------------------------------
module purge
module use /soft/modulefiles
module load gcc/12.2.0
module list
#-------------------------------------------------------------------------------

# get a unique tag
TAG=`date '+%Y-%m-%d-%H%M'`-`uuidgen -t | head -c 4`
# get the dir list
HOME_DIR=${HOME}/racetrack
SCRATCH_DIR=/gpfs/jlse-fs0/users/tgillis/benchme_${TAG}_${COBALT_JOBID}

echo "=================================================================="
echo "SCRATCH_DIR: ${SCRATCH_DIR}"
echo "Job size = ${COBALT_JOBSIZE}"
echo "=================================================================="

for version in mpich ompi
do
    # get everyting needed in a version specific folder
    SCRATCH=${SCRATCH_DIR}/${version}
    mkdir -p ${SCRATCH}
    mkdir -p ${SCRATCH}/build

    echo "SCRATCH: ${SCRATCH}"

    cp -r ${HOME_DIR}/src ${SCRATCH}
    cp -r ${HOME_DIR}/Makefile ${SCRATCH}
    cp -r ${HOME_DIR}/make_arch ${SCRATCH}

    cd ${SCRATCH}
    echo "currently in $(pwd)"

    # get the correct installation dir
if [[ "${version}" == "mpich" ]]
then
    MPI_INSTALL_DIR=${HOME}/lib-MPICH-4.1a1-UCX_1.13.1
fi
if [[ "${version}" == "ompi" ]]
then
    MPI_INSTALL_DIR=${HOME}/lib-OMPI-5.0.0rc9-UCX-1.13.1
fi
if [[ "${version}" == "earlybird" ]]
then
    MPI_INSTALL_DIR=${HOME}/mpich/_inst
fi
    echo "MPI_INSTALL_DIR = ${MPI_INSTALL_DIR}"

    echo "=================================================================="
    echo "compilation..."
    make clean
    MPI_DIR=${MPI_INSTALL_DIR} MPI_VERSION=${version} ARCH_FILE=make_arch/jlse.mak make

    echo "=================================================================="
    NRANKS=2
    NRANKS_PN=1
    NTHREADS=4
    #-----------------------------------------------------------------------
    # MPICH CONFIGURATION
    if [[ "${version}" == "mpich" ]] || [[ "${version}" == "earlybird" ]]
    then
        #export MPIR_CVAR_CH4_NUM_VCIS=${NTHREADS}
        MPI_CONF="-n ${NRANKS} -bind-to core:${NTHREADS} -ppn ${NRANKS_PN}"
    fi
    #-----------------------------------------------------------------------
    # OMPI CONFIGURATION
    if [[ "${version}" == "ompi" ]]
    then
        MPI_CONF="-n ${NRANKS} --npernode ${NRANKS_PN}"
    fi

    #-----------------------------------------------------------------------
    # OPENMP configuration
    export OMP_PROC_BIND=close
    export OMP_PLACES=cores
    export OMP_NUM_THREADS=${NTHREADS}

    #-----------------------------------------------------------------------
    echo "=================================================================="
    echo " ${NRANKS} RANKS - ${NTHREADS} THREADS"
    echo " mpich # of vci: ${MPIR_CVAR_CH4_NUM_VCIS}"
    echo " mpi cmd: ${OMP_CONF} ${MPI_INSTALL_DIR}/bin/mpiexec ${MPI_CONF}"
    #echo " topo debug:"
    #${MPI_INSTALL_DIR}/bin/mpicc -fopenmp src/thread_detect.c
    #${MPI_INSTALL_DIR}/bin/mpiexec ${MPI_CONF} ./a.out > topo_${version}_${NRANKS}_${NTHREADS}
    #sort topo_${version}_${NRANKS}_${NTHREADS}
    #-----------------------------------------------------------------------
    echo "------------------------------------------------------------------"
    ${MPI_INSTALL_DIR}/bin/mpiexec ${MPI_CONF} ./benchme
done

# end of file
