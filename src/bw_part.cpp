/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part.hpp"

void BwPart::RequestInit(TestPartInfo* info) {
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // tag = 0 is already used
    const int tag   = 1;
    const int buddy = get_friend(rank, comm_size);

    n_rqst_ = 1;
    //rqst_   = (MPI_Request*)malloc(sizeof(MPI_Request));
    if (is_sender(rank, comm_size)) {
        MPI_Psend_init(info->buf, n_part, info->size.per_part, MPI_DOUBLE, buddy, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &rqst_);
    } else {
        MPI_Precv_init(info->buf, n_part, info->size.per_part, MPI_DOUBLE, buddy, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &rqst_);
    }
}

void BwPart::Send_StartPreCompute() {
#pragma omp master
    {
        MPI_Start(&rqst_);
    }
#pragma omp barrier
}
void BwPart::Send_Pready(const int i_part) {
    MPI_Pready(i_part, rqst_);
}
void BwPart::Send_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        MPI_Wait(&rqst_, MPI_STATUS_IGNORE);
    }
}

void BwPart::Recv_StartPreCompute() {
#pragma omp master
    {
        MPI_Start(&rqst_);
    }
#pragma omp barrier
}
void BwPart::Recv_Pready(const int i_part) {
    int flag;
    do {
        MPI_Parrived(rqst_, i_part, &flag);
    } while (!flag);
}
void BwPart::Recv_StartPostCompute() {
#pragma omp barrier
#pragma omp master
    {
        MPI_Wait(&rqst_, MPI_STATUS_IGNORE);
    }
}
void BwPart::RequestCleanup(TestPartInfo* info){
    MPI_Request_free(&rqst_);
}
