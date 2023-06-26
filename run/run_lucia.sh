#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

#!/bin/bash
#SBATCH --job-name=benchme
#SBATCH --partition=batch
#SBATCH --nodes=2
#SBATCH --mem=245G
#SBATCH --time=00:10:00
#SBATCH --tasks-per-node=128


#-------------------------------------------------------------------------------
module purge
module load GCC/11.3.0
module list
#-------------------------------------------------------------------------------

# get a unique tag
TAG=`date '+%Y-%m-%d-%H%M'`-`uuidgen -t | head -c 4`
# get the dir list
HOME_DIR=${HOME}/racetrack
SCRATCH_DIR=/gpfs/scratch/betatest/tgillis/benchme_${TAG}_${SLURM_JOBID}

echo "=================================================================="
echo "SCRATCH_DIR: ${SCRATCH_DIR}"
echo "=================================================================="

source run_bw_part.sh

function run_bw(){
	version=${1}
	nthreads=${2}
	nvci=${3}
	global_progress=${4}
	
	# get everyting needed in a version specific folder
	SCRATCH=${SCRATCH_DIR}/${version}-nvci${nvci}
	mkdir -p ${SCRATCH}
	mkdir -p ${SCRATCH}/build
	
	args=
	opts=
	#-----------------------------------------------------------------------
	# MPICH
	if [[ "${version}" == "mpich" ]] || [[ "${version}" == "mpich-vci" ]] || [[ "${version}" == "earlybird" ]]
	then
		args="mpich"
		# get the installation dir
		if [[ "${version}" == "mpich" ]]
		then
			args="${args} ${HOME}/lib-MPICH-4.1-UCX-1.13.1"
		fi
		if [[ "${version}" == "mpich-vci" ]]
		then
			args="${args} ${HOME}/lib-MPICH-4.1-UCX-1.13.1-vci"
		fi
		if [[ "${version}" == "earlybird" ]]
		then
			args="${args} ${HOME}/mpich/_inst"
		fi

		# get the env variables
		opts="MPIR_CVAR_CH4_NUM_VCIS=${nvci}"
		if [[ "${global_progress}" -eq "0" ]]
		then
			opts="${opts} MPIR_CVAR_CH4_GLOBAL_PROGRESS=0"
		fi
	fi
	#-----------------------------------------------------------------------
	# OMPI
	if [[ "${version}" == "ompi" ]]
	then
	     	args="ompi ${HOME}/lib-OMPI-5.0rc10-UCX-1.13.1"
	fi

	#-----------------------------------------------------------------------
	# go there and run
	cp -r ${HOME_DIR}/src ${SCRATCH}
	cp -r ${HOME_DIR}/Makefile ${SCRATCH}
	cp -r ${HOME_DIR}/make_arch ${SCRATCH}
	
	cd ${SCRATCH}
	echo "currently in $(pwd)"

	run_bw_part ${args} ${nthreads} ${opts}
}


# comparison with ompi
# 	version    nthreads  nvci  global progress
run_bw  mpich      16        1     1
run_bw  earlybird  16        1     1
run_bw  ompi       16        1     1

# effects of VCI
run_bw  mpich-vci  16        16    0
run_bw  earlybird  16        16    0


# end of file
