/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include <cxxabi.h>
#include <mpi.h>

#include <cstdio>

#include "bw_dtype_3d.hpp"
#include "bw_pack.hpp"
#include "bw_pack_3d.hpp"
#include "bw_part.hpp"
#include "bw_part_single.hpp"
#include "bw_part_multi.hpp"
#include "bw_part_stream.hpp"
#include "bw_part_rma.hpp"
#include "bw_part_rma_active.hpp"
#include "bw_part_rma_single_active.hpp"
#include "bw_part_rma_single.hpp"
#include "bw_part_rma_fence.hpp"
#include "tools.hpp"

template <class T>
void print_name(T obj) {
    int status;
    printf("type => %s\n",
           abi::__cxa_demangle(typeid(obj).name(), 0, 0, &status));
}

int main(int argc, char * argv[]){
    // init MPI and the Google bench
    //MPI_Init(&argc,&argv);
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    //--------------------------------------------------------------------------
    {
        m_combine(
            /* n_partpt */ m_values(1,2,4,8,16,32),
            /* n_warmup */ m_values(1),
            /* n_repeat */ m_values(150),
            /* max_count */ m_values(1<<22),
            /* noise_level */ m_values(0,10,100)
            ) opt;
        m_test(BwPartSingle, opt);
        //m_test(BwPartStream, opt);
        m_test(BwPartMulti, opt);
        m_test(BwPartRma, opt);
        m_test(BwPartRmaActive, opt);
        m_test(BwPartRmaSingle, opt);
        m_test(BwPartRmaSingleActive, opt);
        m_test(BwPartRmaFence, opt);
        // must be last :-)
        m_test(BwPart, opt);
    }
    //--------------------------------------------------------------------------
    MPI_Finalize();
};

