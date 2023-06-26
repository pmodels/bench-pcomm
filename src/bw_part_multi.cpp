/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_multi.hpp"
#include "mpi.h"

void BwPartMulti::RequestInit(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // tag 0 is already used!
    const int tag   = 1;
    const int buddy = get_friend(rank, comm_size);

    n_rqst_ = n_part;
    rqst_   = (MPI_Request*)malloc(n_part * sizeof(MPI_Request));

    // #pragma omp parallel for schedule(static, 1)
    for (int ip = 0; ip < n_part; ++ip) {
        MPI_Comm part_comm;
        MPI_Comm_dup(MPI_COMM_WORLD, &part_comm);
        if (is_sender(rank, comm_size)) {
            MPI_Send_init(info->buf + ip * info->size.per_part, info->size.per_part, MPI_DOUBLE, buddy, tag + ip, part_comm, rqst_ + ip);
        } else {
            MPI_Recv_init(info->buf + ip * info->size.per_part, info->size.per_part, MPI_DOUBLE, buddy, tag + ip, part_comm, rqst_ + ip);
        }
        MPI_Comm_free(&part_comm);
    }
}

void BwPartMulti::Send_StartPreCompute() {
#pragma omp barrier
}
void BwPartMulti::Send_Pready(const int i_part) {
    MPI_Start(rqst_ + i_part);
}
void BwPartMulti::Send_StartPostCompute() {
#pragma omp for schedule(static) nowait
    for (int ip = 0; ip < n_part; ++ip) {
        MPI_Wait(rqst_ + ip,MPI_STATUS_IGNORE);
    }
#pragma omp barrier
}

void BwPartMulti::Recv_StartPreCompute() {
#pragma omp barrier
#pragma omp for schedule(static) nowait
    for (int ip = 0; ip < n_part; ++ip) {
        MPI_Start(rqst_ + ip);
    }
}
void BwPartMulti::Recv_Pready(const int i_part) {
    MPI_Wait(rqst_ + i_part, MPI_STATUS_IGNORE);
}
void BwPartMulti::Recv_StartPostCompute() {
#pragma omp barrier
}

void BwPartMulti::RequestCleanup(TestPartInfo* info) {
    for (int ip = 0; ip < n_part; ++ip) {
        MPI_Request_free(rqst_ + ip);
    }
    free(rqst_);
}
