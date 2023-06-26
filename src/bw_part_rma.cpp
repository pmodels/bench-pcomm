/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_rma.hpp"
#include "mpi.h"
#include "mpi_proto.h"

void BwPartRma::RequestInit(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    const int buddy     = get_friend(rank, comm_size);
    const int n_threads = omp_get_max_threads();
    put_info_           = (put_info_t*)malloc(n_threads * sizeof(put_info_t));

    // get the new group and the new comm associated to the send/recv ranks only!
    MPI_Group comm_group;
    MPI_Comm_group(MPI_COMM_WORLD, &comm_group);

    int rank_list[2];
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    rank_list[0] = m_min(my_rank,buddy);
    rank_list[1] = m_max(my_rank,buddy);
    MPI_Comm  win_comm;
    MPI_Group win_group;
    MPI_Group_incl(comm_group, 2, rank_list, &win_group);
    MPI_Comm_create_group(MPI_COMM_WORLD, win_group, 0, &win_comm);

    /* one window per thread covering the whole data buffer! */
    int target_rank;
    MPI_Group_translate_ranks(comm_group, 1, &buddy, win_group, &target_rank);
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_[ip].target_rank = target_rank;
        put_info_[ip].count       = info->size.per_part;
        put_info_[ip].buf         = info->buf; //+ info->size.per_part * ip;

        // duplicate the comms to use different VCIs, each window on a different comm
        MPI_Info win_info;
        MPI_Info_create(&win_info);
        MPI_Info_set(win_info, "same_disp_unit", "true");
        MPI_Comm_dup(win_comm, &(put_info_[ip].comm));
        if (is_sender(rank, comm_size)) {
            // size is 0 on the sender, address can then be NULL
            MPI_Win_create(NULL, 0, 1, win_info, put_info_[ip].comm, &(put_info_[ip].win));
            MPI_Win_lock(MPI_LOCK_SHARED, put_info_[ip].target_rank, MPI_MODE_NOCHECK, put_info_[ip].win);
        } else {
            MPI_Aint win_size = info->size.per_part * n_part * sizeof(double);
            MPI_Win_create(put_info_[ip].buf, win_size, 1, win_info, put_info_[ip].comm, &(put_info_[ip].win));
        }
    }
    /* free the groups, comms etc */
    MPI_Group_free(&comm_group);
    MPI_Group_free(&win_group);
    MPI_Comm_free(&win_comm);
}

void BwPartRma::Send_StartPreCompute() {
    // 0-byte send/recv to notify exposure, on the first communicator.
    // recv will complete once the send has been issued. I know that my recver is ready at that time then
#pragma omp master
    {
        const int ith = omp_get_thread_num();
        int       tag = 1;
        MPI_Recv(NULL, 0, MPI_BYTE, put_info_[ith].target_rank, tag, put_info_[ith].comm, MPI_STATUS_IGNORE);
    }
#pragma omp barrier
}
void BwPartRma::Send_Pready(const int i_part) {
    const int   ith  = omp_get_thread_num();
    put_info_t* info = put_info_ + ith;

    int      count = info->count;
    MPI_Aint disp  = info->count * i_part * sizeof(double);
    void*    buf   = (char*)info->buf + disp;
    MPI_Put(buf, count, MPI_DOUBLE, info->target_rank, disp, info->count, MPI_DOUBLE, info->win);
}
void BwPartRma::Send_StartPostCompute() {
    // all the threads will flush
    const int n_threads = omp_get_max_threads();
#pragma omp for schedule(static)
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_t* info = put_info_ + ip;
        MPI_Win_flush(info->target_rank, info->win);
    }
    // 0-byte send/recv to notify completion
    // no need to use Ssend as we don't want to wait for completion on the recv
#pragma omp barrier
#pragma omp master
    {
        const int ith = omp_get_thread_num();
        int       tag = 2;
        MPI_Send(NULL, 0, MPI_BYTE, put_info_[ith].target_rank, tag, put_info_[ith].comm);
    }
}

void BwPartRma::Recv_StartPreCompute() {
    // indicate RTS to the sender
#pragma omp master
    {
        const int ith = omp_get_thread_num();
        int       tag = 1;
        MPI_Send(NULL, 0, MPI_BYTE, put_info_[ith].target_rank, tag, put_info_[ith].comm);
    }
#pragma omp barrier
}
void BwPartRma::Recv_Pready(const int i_part) {}
void BwPartRma::Recv_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        const int ith = omp_get_thread_num();
        int       tag = 2;  // 0 is used by the handshake
        MPI_Recv(NULL, 0, MPI_BYTE, put_info_[ith].target_rank, tag, put_info_[ith].comm, MPI_STATUS_IGNORE);
    }
}

void BwPartRma::RequestCleanup(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    const int n_threads = omp_get_max_threads();
    for (int ip = 0; ip < n_threads; ++ip) {
        if (is_sender(rank, comm_size)) {
            MPI_Win_unlock(put_info_[ip].target_rank, put_info_[ip].win);
        }
        MPI_Win_free(&(put_info_[ip].win));
        MPI_Comm_free(&(put_info_[ip].comm));
    }
    free(put_info_);
}
