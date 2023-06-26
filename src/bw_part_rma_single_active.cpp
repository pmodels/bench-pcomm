/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_rma_single_active.hpp"
#include "mpi.h"

void BwPartRmaSingleActive::RequestInit(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    const int buddy = get_friend(rank, comm_size);
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

    put_info_.target_rank = target_rank;
    put_info_.count       = info->size.per_part;
    put_info_.buf         = info->buf;

    // create the communicator group for the active sync
    MPI_Group_incl(comm_group, 1, &buddy, &(put_info_.group));

    // duplicate the comms to use different VCIs, each window on a different comm
    MPI_Info win_info;
    MPI_Info_create(&win_info);
    MPI_Info_set(win_info, "same_disp_unit", "true");
    MPI_Comm_dup(win_comm, &(put_info_.comm));
    if (is_sender(rank, comm_size)) {
        // size is 0 on the sender, address can then be NULL
        MPI_Win_create(NULL, 0, 1, win_info, put_info_.comm, &(put_info_.win));
    } else {
        MPI_Aint win_size = info->size.per_part * sizeof(double);
        MPI_Win_create(put_info_.buf, win_size, 1, win_info, put_info_.comm, &(put_info_.win));
    }

    /* free the groups, comms etc */
    MPI_Group_free(&comm_group);
    MPI_Group_free(&win_group);
    MPI_Comm_free(&win_comm);
}

void BwPartRmaSingleActive::Send_StartPreCompute() {
#pragma omp master
    {
        // sender starts an access epoch
        MPI_Win_start(put_info_.group, 0, put_info_.win);
    }
#pragma omp barrier
}
void BwPartRmaSingleActive::Send_Pready(const int i_part) {
    MPI_Aint disp = i_part * put_info_.count * sizeof(double);
    void*    buf  = ((char*)put_info_.buf) + disp;
    MPI_Put(buf, put_info_.count, MPI_DOUBLE, put_info_.target_rank, 0, put_info_.count, MPI_DOUBLE, put_info_.win);
}
void BwPartRmaSingleActive::Send_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        // close the access epoch
        MPI_Win_complete(put_info_.win);
    }
}

void BwPartRmaSingleActive::Recv_StartPreCompute() {
#pragma omp master
    {
        // start the exposure epoch
        MPI_Win_post(put_info_.group, 0, put_info_.win);
    }
#pragma omp barrier
}
void BwPartRmaSingleActive::Recv_Pready(const int i_part) {
}
void BwPartRmaSingleActive::Recv_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        MPI_Win_wait(put_info_.win);
    }
}

void BwPartRmaSingleActive::RequestCleanup(TestPartInfo* info) {
    MPI_Win_free(&(put_info_.win));
    MPI_Comm_free(&(put_info_.comm));
    MPI_Group_free(&(put_info_.group));
}
