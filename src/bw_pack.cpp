/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_pack.hpp"
#include <cstring>

// we send bcount * bsize doubles continuous in memory
void BwPack::SetupComm(const int n_msg, const int max_count, BwDtypeInfo* info) {
    info->dcount = bcount * bsize;
    info->dtype  = MPI_DOUBLE;
    // we allocate the block count * the size (the memory can be contiguous
    info->alloc_byte = bcount * bsize * sizeof(double);
    // we will communicate the block count * the block size
    info->comm_byte = bcount * bsize * sizeof(double);
    // allocate the usr buffer
    const size_t usr_size = n_msg * max_count * bcount * bstride * sizeof(double);
    usr_buf               = (char*)malloc(usr_size);

    m_log("allocation size on the usr = %f MB",usr_size/1.0e+6);
}
void BwPack::UpsetComm(const int max_count, const int n_info, BwDtypeInfo* info) {
    // free the buffer
    free(usr_buf);
}

// do the packing from the usr buffer to buff
void BwPack::PreSend(const int i_msg, const int count, char* buf) {
    const size_t usr_shift = i_msg * count * bstride;
    for (int i = 0; i < (count * bcount); ++i) {
        // comm buff increments by bsize, user_buf increment by bstride
        double* l_buf     = ((double*)buf) + i * bsize;
        double* l_usr_buf = ((double*)usr_buf) + usr_shift + i * bstride;
        std::memcpy(l_usr_buf, l_buf, bsize);
    }
}
// do the unpacking from buf to the usr buffer
void BwPack::PostRecv(const int i_msg, const int count, char* buf) {
    const size_t usr_shift = i_msg * count * bstride;
    for (int i = 0; i < (count * bcount); ++i) {
        // comm buff increments by bsize, user_buf increment by bstride
        double* l_buf     = ((double*)buf) + i * bsize;
        double* l_usr_buf = ((double*)usr_buf) + usr_shift + i * bstride;
        std::memcpy(l_buf, l_usr_buf, bsize);
    }
}
