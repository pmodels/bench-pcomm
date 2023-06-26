#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

#!/bin/bash -l
#SBATCH --partition=cpu
#SBATCH --time=4:00:00
#SBATCH --nodes=2
#SBATCH --account=p200067
#SBATCH --qos=short
#SBATCH --switches=1
#SBATCH --exclude=mel[0015,0017,0141,0142,0314,0315,0341,0342,0429,0431]

#-------------------------------------------------------------------------------
module purge
module use env/release/2022.1
module load GCC
module load Automake Autoconf libtool
module list
#-------------------------------------------------------------------------------

# get a unique tag
TAG=`date '+%Y-%m-%d-%H%M'`-`uuidgen -t | head -c 4`
# get the dir list
HOME_DIR=${HOME}/racetrack
SCRATCH_DIR=/project/scratch/p200067/benchme_${TAG}_${SLURM_JOBID}

echo "=================================================================="
echo "SCRATCH_DIR: ${SCRATCH_DIR}"
echo "=================================================================="

source run_bw_part.sh

function run_bw(){
	version=${1}
	nthreads=${2}
	nvci=${3}
	global_progress=${4}
	aggregate_size=${5}
	
	# get everyting needed in a version specific folder
	SCRATCH=${SCRATCH_DIR}/${version}-threads${nthreads}-nvci${nvci}-aggr${aggregate_size}
	mkdir -p ${SCRATCH}
	mkdir -p ${SCRATCH}/build
	
	args=
	opts=
	#-----------------------------------------------------------------------
	# MPICH
	if [[ "${version}" == "mpich" ]] || [[ "${version}" == "mpich-vci" ]] || [[ "${version}" == "earlybird" ]] || [[ "${version}" == "earlybird-tag" ]]
	then
		args="mpich"
		# get the installation dir
		if [[ "${version}" == "mpich" ]]
		then
			args="${args} ${HOME}/lib-MPICH-4.1-UCX-1.13.1"
		fi
		if [[ "${version}" == "mpich-vci" ]]
		then
			#args="${args} ${HOME}/lib-MPICH-4.1-UCX-1.13.1-vci-timer"
			args="${args} ${HOME}/lib-MPICH-4.1-UCX-1.13.1-vci"
		fi
		if [[ "${version}" == "earlybird" ]]
		then
			args="${args} ${HOME}/mpich/_inst"
		fi
		if [[ "${version}" == "earlybird-tag" ]]
		then
			args="${args} ${HOME}/mpich/_inst_tag"
		fi

		# get the env variables
		#opts="MPIR_CVAR_CH4_RESERVE_VCIS=${nthreads} MPIR_CVAR_CH4_NUM_VCIS=${nvci} MPIR_CVAR_PART_AGGR_SIZE=${aggregate_size}"
		opts="MPIR_CVAR_CH4_NUM_VCIS=${nvci} MPIR_CVAR_PART_AGGR_SIZE=${aggregate_size}"
		#opts="MPIR_CVAR_CH4_NUM_VCIS=${nvci} MPIR_CVAR_PART_AGGR_SIZE=${aggregate_size}"
		if [[ "${global_progress}" -eq "0" ]]
		then
			opts="${opts} MPIR_CVAR_CH4_GLOBAL_PROGRESS=0"
        else
			opts="${opts} MPIR_CVAR_CH4_GLOBAL_PROGRESS=1"
		fi
	fi
	#-----------------------------------------------------------------------
	# OMPI
	if [[ "${version}" == "ompi" ]] || [[ "${version}" == "ompi-nobind" ]]
	then
	    #args="ompi ${HOME}/lib-OMPI-5.0.0rc10-UCX-1.13.1"
	    args="ompi ${HOME}/lib-OMPI-5.0.0rc9-UCX-1.13.1"
		#if [[ "${version}" == "ompi" ]]
		#then
		#	opts="PRTE_MCA_rmaps_default_mapping_policy=ppr:1:node:pe=${nthreads}"
		#fi
	fi
	if [[ "${version}" == "ompi-nobind" ]]
	then
	    #args="ompi ${HOME}/lib-OMPI-5.0.0rc10-UCX-1.13.1"
	    args="ompi-nobind ${HOME}/lib-OMPI-5.0.0rc9-UCX-1.13.1"
		#if [[ "${version}" == "ompi" ]]
		#then
		#	opts="PRTE_MCA_rmaps_default_mapping_policy=ppr:1:node:pe=${nthreads}"
		#fi
	fi

	#-----------------------------------------------------------------------
	# go there and run
	cp -r ${HOME_DIR}/src ${SCRATCH}
	cp -r ${HOME_DIR}/Makefile ${SCRATCH}
	cp -r ${HOME_DIR}/make_arch ${SCRATCH}
	
	cd ${SCRATCH}
	echo "currently in $(pwd)"
    echo "calling run_bw_part with : ${args} ${nthreads} ${opts}"

	run_bw_part ${args} ${nthreads} ${opts}
}

# baseline - 1 thread
# congestion - x threads
# VCI - x threads, x VCI
#for thread in 2
for thread in 1 2 4 8 16 32
do
    ##      version         nthreads    nvci    glob progress   aggregate size
    run_bw  mpich-vci       ${thread}  ${thread}    0               0
    run_bw  earlybird-tag   ${thread}  ${thread}    0               0
    ###      version         nthreads    nvci    glob progress   aggregate size
    run_bw  mpich-vci       ${thread}   1           0               0
    run_bw  earlybird-tag   ${thread}   1           0               0
done

# aggregation
#for size in 0 512
for size in 0 512 1024 2048 4096 16384
do
    ##      version         nthreads    nvci    glob progress   aggregate size
    run_bw  mpich-vci       1   1           0               ${size}
    run_bw  earlybird-tag   1   1           0               ${size}
done

# end of file

