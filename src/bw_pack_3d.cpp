/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_pack_3d.hpp"
#include <cstring>

// we send bcount * bsize doubles continuous in memory
void BwPack3D::SetupComm(const int n_msg, const int max_count, BwDtypeInfo* info) {
    info->dcount = bn[0] * bn[1] * bn[2];
    info->dtype  = MPI_DOUBLE;
    // we allocate the block count * the size (the memory can be contiguous
    info->alloc_byte = bn[0] * bn[1] * bn[2] * sizeof(double);
    // we will communicate the block count * the block size
    info->comm_byte = bn[0] * bn[1] * bn[2] * sizeof(double);
    // allocate the usr buffer
    const size_t usr_size = n_msg * max_count * n[0] * n[1] * n[2]* sizeof(double);
    usr_buf               = (char*)malloc(usr_size);

    m_log("allocation size on the usr = %f MB",usr_size/1.0e+6);
}
void BwPack3D::UpsetComm(const int max_count, const int n_info, BwDtypeInfo* info) {
    // free the buffer
    free(usr_buf);
}

// do the packing from the usr buffer to buff
void BwPack3D::PreSend(const int i_msg, const int count, char* buf) {
    const size_t  usr_shift = i_msg * count * n[0] * n[1] * n[2];
    const double* ubuf      = ((double*)(usr_buf)) + usr_shift;
    double*       cbuf      = ((double*)(buf));

    for (int ic = 0; ic < count; ++ic) {
        for (int i2 = 0; i2 < bn[2]; ++i2) {
            for (int i1 = 0; i1 < bn[1]; ++i1) {
                // we use memcopy for the bn[0] continuous elements
                const int uidx = n[0] * (i1 + n[1] * (i2 + n[2] * ic));
                const int cidx = bn[0] * (i1 + bn[1] * (i2 + bn[2] * ic));
                std::memcpy(cbuf + cidx, ubuf + uidx, bn[0] * sizeof(double));
            }
        }
    }
}
// do the unpacking from buf to the usr buffer
void BwPack3D::PostRecv(const int i_msg, const int count, char* buf) {
    const size_t  usr_shift = i_msg * count * n[0] * n[1] * n[2];
    double*       ubuf      = ((double*)(usr_buf)) + usr_shift;
    const double* cbuf      = ((double*)(buf));

    for (int ic = 0; ic < count; ++ic) {
        for (int i2 = 0; i2 < bn[2]; ++i2) {
            for (int i1 = 0; i1 < bn[1]; ++i1) {
                // we use memcopy for the bn[0] continuous elements
                const int uidx = n[0] * (i1 + n[1] * (i2 + n[2] * ic));
                const int cidx = bn[0] * (i1 + bn[1] * (i2 + bn[2] * ic));
                std::memcpy(ubuf + uidx, cbuf + cidx, bn[0] * sizeof(double));
            }
        }
    }
}

