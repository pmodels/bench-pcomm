/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef TEST_BW_HPP_
#define TEST_BW_HPP_

#include <mpi.h>
#include <tuple>
#include "tools.hpp"
#include <cstdio>
#include <omp.h>
#include <iostream>
#include <cstdlib>

using part_arg_t       = std::tuple<int, int, int, int, int>;
constexpr int part_arg_s = std::tuple_size_v<part_arg_t>;

// Describes the partition layout used in BW test
struct BwPartInfo {
    int          npart = 1;                  // the number of partitions
    int          count = 1;                  // the number of MPI_Datatype within one partition
    MPI_Datatype dtype = MPI_DATATYPE_NULL;  // the MPI datatype to use for the partitions
};

typedef struct TestPartInfo{
    struct {
    //int rqst; // the number of requests
    size_t per_rank; // the count of data per rank
    size_t per_part; // the count of data per partition
    size_t bandwidth;
    } size;
    //MPI_Request* rqst;
    double* buf;
}TestPartInfo;

/* Test the BANDWITH of the network
 *
 * The test repeat the following experiment n_repeat times:
 * send n_msg each containing count BwDtypes,
 * then wait for the handshake to indicate completion on the receiver side
 *
 * One BwDtype is described by the BwDtypeInfo structure.
 * From the MPI perspective it's dcount times the associated MPI_Datatype.
 *
 */
class TestPartBw {
   protected:
    const int  n_partpt;   // number of partitions per thread
    const int  n_warmup;   // number of warmup iterations to do
    const int  n_repeat;   // number of iterations to do after the warmup ones
    const int  max_count;  // the maximum number of dtypes exchanged
    const int  noise_lvl;  // the noise expressed as 1e-6 sec/MB of memory per partition
    BwPartInfo bw_dtype;   // the BwDtype exchanged

    int n_threads;
    int n_part;

   public:
    //--------------------------------------------------------------------------
    TestPartBw() = delete;
    explicit TestPartBw(part_arg_t arg) : n_partpt{std::get<0>(arg)},
                                          n_warmup{std::get<1>(arg)},
                                          n_repeat{std::get<2>(arg)},
                                          max_count{std::get<3>(arg)},
                                          noise_lvl{std::get<4>(arg)} {
        n_threads = omp_get_max_threads();
        n_part = n_partpt * n_threads;
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD,&rank);
        if (!rank) {
            m_log("------------------------------------");
            m_info(arg);
            m_log("%d threads - %d parts", n_threads, n_part);
            m_log("------------------------------------");
        }
        /* make sure we don't oversubscribe the number of threads compared to what slurm has for us */
        //int rank;
        //MPI_Comm_rank(MPI_COMM_WORLD,&rank);
        //const char* env = std::getenv("OMP_NUM_THREADS");
        //if(env){
        //    int max_threads = atoi(env);
        //    m_assert(n_threads <=  max_threads, "you are requesting more threads than the one available: %d/%d",max_threads,n_threads);
        //}
        //else{
        //    omp_set_num_threads(n_threads);
        //}
    };

    // run this test
    void run();

   protected:
    virtual void FileName(int len, char* filename) {
       snprintf(filename, len, "%dthreads_%dparts_%dnoise.txt", n_threads, n_part, noise_lvl);
    };

    virtual void RequestInit(TestPartInfo* info)    = 0;
    virtual void Send_StartPreCompute()             = 0;
    virtual void Send_Pready(const int i_part)      = 0;
    virtual void Send_StartPostCompute()            = 0;
    virtual void Recv_StartPreCompute()             = 0;
    virtual void Recv_Pready(const int i_part)      = 0;
    virtual void Recv_StartPostCompute()            = 0;
    virtual void RequestCleanup(TestPartInfo* info) = 0;

   private:
    void get_noise_(const int npart, double* noise);
};

#endif
