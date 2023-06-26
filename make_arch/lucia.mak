#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

# JLSE

MPI_DIR ?= ${HOME}/lib-MPICH-4.1-UCX_1.13.1

CC =  ${MPI_DIR}/bin/mpicc
CXX = ${MPI_DIR}/bin/mpicxx

CXXFLAGS = -g -O3 -fopenmp #-fsanitize=address
LDFLAGS = -fopenmp #-fsanitize=address

