/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_single.hpp"

void BwPartSingle::RequestInit(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // tag == 0 is already used
    const int tag   = 1;
    const int buddy = get_friend(rank, comm_size);

    const int count = info->size.per_part * n_part;
    if (is_sender(rank, comm_size)) {
        MPI_Send_init(info->buf, count, MPI_DOUBLE, buddy, tag, MPI_COMM_WORLD, &rqst_);
    } else {
        MPI_Recv_init(info->buf, count, MPI_DOUBLE, buddy, tag, MPI_COMM_WORLD, &rqst_);
    }
}

void BwPartSingle::Send_StartPreCompute() {
#pragma omp barrier
}
void BwPartSingle::Send_Pready(const int i_part) {}
void BwPartSingle::Send_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        MPI_Start(&rqst_);
        MPI_Wait(&rqst_, MPI_STATUS_IGNORE);
    }
}

void BwPartSingle::Recv_StartPreCompute() {
#pragma omp master
    {
        MPI_Start(&rqst_);
        MPI_Wait(&rqst_, MPI_STATUS_IGNORE);
    }
}
void BwPartSingle::Recv_Pready(const int i_part) {}
void BwPartSingle::Recv_StartPostCompute() {
#pragma omp barrier
}
void BwPartSingle::RequestCleanup(TestPartInfo* info) {}
