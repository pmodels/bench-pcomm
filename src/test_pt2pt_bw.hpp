/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef TEST_BW_HPP
#define TEST_BW_HPP

#include <mpi.h>

#include <tuple>

#include "tools.hpp"

using bw_arg_t         = std::tuple<int, int, int>;
constexpr int bw_arg_s = std::tuple_size_v<bw_arg_t>;

// Describes the datatype as send by BW test
// a BwDtype is dcount MPI_Datatypes continuous in memory
struct BwDtypeInfo {
    int    dcount     = 1;  // the number of MPI_Datatype within one BwDtype
    MPI_Datatype dtype = MPI_DATATYPE_NULL;  // the MPI datatype to use underneath

    // describe the memory need and communicated:
    size_t alloc_byte = 0;  // the size of one BwDtype in byte (to be allocated!)
    size_t comm_byte  = 0;  // the size of one BwDtype in byte (the size actually communicated)
};

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
class TestPt2PtBw {
   private:
    const int n_msg;      // number of msgs to send
    const int n_repeat;   // number of repetition to do
    const int max_count;  // the maximum number of dtypes exchanged
    BwDtypeInfo bw_dtype;  // the BwDtype exchanged

   public:
    //--------------------------------------------------------------------------
    TestPt2PtBw() = delete;
    explicit TestPt2PtBw(bw_arg_t arg) : n_msg{std::get<1>(arg)},
                                         n_repeat{std::get<2>(arg)},
                                         max_count{std::get<0>(arg)} {
        m_info(arg);
    };

    // run this test
    void run();


    //--------------------------------------------------------------------------
    // virtual function
    // returns the filename to be used, can be override
    virtual void Filename(const int len, char *name)=0;

    // Fills the communication information:
    // - n_msg is the number of msgs send each time
    // - BwDtypeInfo is the description of the BwDtype
    // - max_count is the maximum number of BwDtype sent
    virtual void SetupComm(const int n_msg, const int max_count, BwDtypeInfo* info) = 0;
    virtual void UpsetComm(const int n_msg, const int max_count, BwDtypeInfo* info) = 0;

    // define the operation to be done before the send/after the recv
    // NOTE: these functions are taken into account in the timer!
    virtual void PreSend(const int i_msg, const int count, char* buf)  = 0;
    virtual void PostRecv(const int i_msg, const int count, char* buf) = 0;
};

#endif
