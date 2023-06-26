/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_rma_single.hpp"
#include "mpi.h"

void BwPartRmaSingle::RequestInit(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    const int buddy = get_friend(rank, comm_size);
    put_info_       = (put_info_t*)malloc(sizeof(put_info_t));

    // get the new group and the new comm associated to the send/recv ranks only!
    MPI_Group comm_group;
    MPI_Comm_group(MPI_COMM_WORLD, &comm_group);

    int rank_list[2];
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    rank_list[0] = m_min(my_rank,buddy);
    rank_list[1] = m_max(my_rank,buddy);
    MPI_Group win_group;
    MPI_Group_incl(comm_group, 2, rank_list, &win_group);
    MPI_Comm_create_group(MPI_COMM_WORLD, win_group, 0, &(put_info_->comm));

    /* store the new rank id */
    int target_rank;
    MPI_Group_translate_ranks(comm_group, 1, &buddy, win_group, &target_rank);

    put_info_->target_rank = target_rank;
    put_info_->count       = info->size.per_part;
    put_info_->buf         = info->buf;

    // duplicate the comms to use different VCIs, each window on a different comm
    MPI_Info win_info;
    MPI_Info_create(&win_info);
    MPI_Info_set(win_info, "same_disp_unit", "true");
    if (is_sender(rank, comm_size)) {
        // size is 0 on the sender, address can then be NULL
        MPI_Win_create(NULL, 0, 1, win_info, put_info_->comm, &(put_info_->win));
        MPI_Win_lock(MPI_LOCK_SHARED, put_info_->target_rank, MPI_MODE_NOCHECK, put_info_->win);
    } else {
        MPI_Aint win_size = info->size.per_part * n_part * sizeof(double);
        MPI_Win_create(put_info_->buf, win_size, 1, win_info, put_info_->comm, &(put_info_->win));
    }
    /* free the groups, comms etc */
    MPI_Group_free(&comm_group);
    MPI_Group_free(&win_group);
}

void BwPartRmaSingle::Send_StartPreCompute() {
#pragma omp master
    {
        // 0-byte send/recv to notify exposure, on the first communicator.
        // recv will complete once the send has been issued. I know that my recver is ready at that time then
        int tag = 1;  // 0 is used by the handshake
        MPI_Recv(NULL, 0, MPI_BYTE, put_info_->target_rank, tag, put_info_->comm, MPI_STATUS_IGNORE);
    }
#pragma omp barrier
}

void BwPartRmaSingle::Send_Pready(const int i_part) {
    put_info_t* info = put_info_;
    MPI_Aint    disp = i_part * info->count * sizeof(double);
    void*       buf  = ((char*)info->buf) + disp;
    MPI_Put(buf, info->count, MPI_DOUBLE, info->target_rank, disp, info->count, MPI_DOUBLE, info->win);
}
void BwPartRmaSingle::Send_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        MPI_Win_flush(put_info_->target_rank, put_info_->win);
        // 0-byte send/recv to notify completion
        // no need to use Ssend as we don't want to wait for completion on the recv
        int tag = 2;
        MPI_Send(NULL, 0, MPI_BYTE, put_info_[0].target_rank, tag, put_info_[0].comm);
    }
}

void BwPartRmaSingle::Recv_StartPreCompute() {
#pragma omp master
    {
        // indicate RTS to the sender
        int tag = 1;  // 0 is used by the handshake
        MPI_Send(NULL, 0, MPI_BYTE, put_info_->target_rank, tag, put_info_->comm);
    }
#pragma omp barrier
}
void BwPartRmaSingle::Recv_Pready(const int i_part) {
}
void BwPartRmaSingle::Recv_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        int tag = 2;  // 0 is used by the handshake
        MPI_Recv(NULL, 0, MPI_BYTE, put_info_[0].target_rank, tag, put_info_[0].comm, MPI_STATUS_IGNORE);
    }
}

void BwPartRmaSingle::RequestCleanup(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    if (is_sender(rank, comm_size)) {
        MPI_Win_unlock(put_info_->target_rank, put_info_->win);
    }
    MPI_Win_free(&(put_info_->win));
    MPI_Comm_free(&(put_info_->comm));
    free(put_info_);
}
