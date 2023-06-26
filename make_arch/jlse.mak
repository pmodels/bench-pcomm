#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

# JLSE

MPI_DIR ?= ${HOME}/lib-MPICH-4.1a1-UCX_1.13.1

CC =  ${MPI_DIR}/bin/mpicc
CXX = ${MPI_DIR}/bin/mpicxx

CXXFLAGS = -g -O3 #-fsanitize=address
#LDFLAGS = -fsanitize=address

