#
# Copyright (C) by Argonne National Laboratory
#	See COPYRIGHT in top-level directory

#-------------------------------------------------------------------------------
# compilers etc
CC = ${HOME}/dbs_lib/lib_MPICH-4.1-OFI-1.16.1/bin/mpicc
CXX = ${HOME}/dbs_lib/lib_MPICH-4.1-OFI-1.16.1/bin/mpicxx

CXXFLAGS = -fopenmp -O3 -fsanitize=address -Wno-deprecated-declarations -Wno-format-security
LDFLAGS = -fopenmp -fsanitize=address 

#-------------------------------------------------------------------------------
