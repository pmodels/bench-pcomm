/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_rma_fence.hpp"
#include "mpi.h"

void BwPartRmaFence::RequestInit(TestPartInfo* info) {
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

    /* store the new rank id */
    int target_rank;
    MPI_Group_translate_ranks(comm_group, 1, &buddy, win_group, &target_rank);
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_[ip].target_rank = target_rank;
        put_info_[ip].count       = info->size.per_part;
        put_info_[ip].buf         = info->buf;//+ info->size.per_part * ip;

        // duplicate the comms to use different VCIs, each window on a different comm
        MPI_Info win_info;
        MPI_Info_create(&win_info);
        MPI_Info_set(win_info, "same_disp_unit", "true");
        MPI_Comm_dup(win_comm, &(put_info_[ip].comm));
        if (is_sender(rank, comm_size)) {
            // size is 0 on the sender, address can then be NULL
            MPI_Win_create(NULL, 0, 1, win_info, put_info_[ip].comm, &(put_info_[ip].win));
        } else {
            MPI_Aint win_size = info->size.per_part * n_part * sizeof(double);
            MPI_Win_create(put_info_[ip].buf, win_size, 1, win_info, put_info_[ip].comm, &(put_info_[ip].win));
        }
        MPI_Win_fence(0,put_info_[ip].win);
    }
    /* free the groups, comms etc */
    MPI_Group_free(&comm_group);
    MPI_Group_free(&win_group);
    MPI_Comm_free(&win_comm);
}

void BwPartRmaFence::Send_StartPreCompute() {
    // close the access epoch
    const int n_threads = omp_get_max_threads();
#pragma omp barrier
#pragma omp for schedule(static) nowait
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_t* info = put_info_ + ip;
        MPI_Win_fence(0, info->win);
    }
}
void BwPartRmaFence::Send_Pready(const int i_part) {
    const int   ith  = omp_get_thread_num();
    put_info_t* info = put_info_ + ith;

    int      count = info->count;
    MPI_Aint disp  = info->count * i_part * sizeof(double);
    void*    buf   = (char*)info->buf + disp;
    MPI_Put(buf, count, MPI_DOUBLE, info->target_rank, disp, info->count, MPI_DOUBLE, info->win);
}
void BwPartRmaFence::Send_StartPostCompute() {
    const int n_threads = omp_get_max_threads();
#pragma omp for schedule(static) nowait
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_t* info = put_info_ + ip;
        MPI_Win_fence(0, info->win);
    }
#pragma omp barrier
}

void BwPartRmaFence::Recv_StartPreCompute() {
    const int n_threads = omp_get_max_threads();
#pragma omp barrier
#pragma omp for schedule(static) nowait
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_t* info = put_info_ + ip;
        MPI_Win_fence(0, info->win);
    }
}
void BwPartRmaFence::Recv_Pready(const int i_part) {
}
void BwPartRmaFence::Recv_StartPostCompute() {
    const int n_threads = omp_get_max_threads();
#pragma omp for schedule(static) nowait
    for (int ip = 0; ip < n_threads; ++ip) {
        put_info_t* info = put_info_ + ip;
        MPI_Win_fence(0, info->win);
    }
#pragma omp barrier
}

void BwPartRmaFence::RequestCleanup(TestPartInfo* info) {
    const int n_threads = omp_get_max_threads();
    for (int ip = 0; ip < n_threads; ++ip) {
        MPI_Win_free(&(put_info_[ip].win));
        MPI_Comm_free(&(put_info_[ip].comm));
    }
    free(put_info_);
}
