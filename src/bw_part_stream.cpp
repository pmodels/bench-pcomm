/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_part_stream.hpp"
#include <thread>
#include "mpi.h"

void BwPartStream::RequestInit(TestPartInfo* info) {
#ifdef MPICH
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // tag 0 is already used!
    const int tag   = 1;
    const int buddy = get_friend(rank, comm_size);

    // get one stream per thread
    const int n_threads = omp_get_max_threads();
    streams_            = (MPIX_Stream*)malloc(n_threads * sizeof(MPIX_Stream));
    thread_comms_       = (MPI_Comm*)malloc(n_threads * sizeof(MPI_Comm));
    MPI_Errhandler err_handler;
    MPI_Comm_get_errhandler(MPI_COMM_WORLD, &err_handler);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    n_streams_ = 0;
    for (int i = 0; i < n_threads; ++i) {
        int err = MPIX_Stream_create(MPI_INFO_NULL, streams_ + i);
        if (err == MPI_SUCCESS) {
            MPIX_Stream_comm_create(MPI_COMM_WORLD, streams_[i], thread_comms_ + i);
            n_streams_++;
        } else {
            MPIX_Stream_comm_create(MPI_COMM_WORLD, MPIX_STREAM_NULL, thread_comms_ + i);
        }
    }
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, err_handler);

    // request are created for each partition
    n_rqst_ = n_part;
    rqst_   = (MPI_Request*)malloc(n_part * sizeof(MPI_Request));
#pragma omp parallel for schedule(static)
    for (int ip = 0; ip < n_part; ++ip) {
        int       ith  = omp_get_thread_num();
        MPI_Comm* comm = thread_comms_ + ith;
        if (is_sender(rank, comm_size)) {
            MPI_Send_init(info->buf + ip * info->size.per_part, info->size.per_part, MPI_DOUBLE, buddy, tag + ip, *comm, rqst_ + ip);
        } else {
            MPI_Recv_init(info->buf + ip * info->size.per_part, info->size.per_part, MPI_DOUBLE, buddy, tag + ip, *comm, rqst_ + ip);
        }
    }
#endif
}

void BwPartStream::Send_StartPreCompute() {}
void BwPartStream::Send_Pready(const int i_part) {
#ifdef MPICH
    MPI_Start(rqst_ + i_part);
#endif
}
void BwPartStream::Send_StartPostCompute() {
    int is_done;
    do {
        is_done = 1;
#pragma omp for schedule(static) nowait
        for (int ip = 0; ip < n_rqst_; ++ip) {
            int flag;
            MPI_Test(rqst_ + ip, &flag, MPI_STATUS_IGNORE);
            is_done = is_done && flag;
        }
    } while (!is_done);
}

void BwPartStream::Recv_StartPreCompute() {
#pragma omp for schedule(static)
    for (int ip = 0; ip < n_rqst_; ++ip) {
        MPI_Start(rqst_ + ip);
    }
}

void BwPartStream::Recv_Pready(const int i_part) {}

void BwPartStream::Recv_StartPostCompute() {
    int is_done;
    do {
        is_done = 1;
#pragma omp for schedule(static) nowait
        for (int ip = 0; ip < n_rqst_; ++ip) {
            int flag;
            MPI_Test(rqst_ + ip, &flag, MPI_STATUS_IGNORE);
            is_done = is_done && flag;
        }
    } while (!is_done);
}

void BwPartStream::RequestCleanup(TestPartInfo* info) {
#ifdef MPICH
    for (int ip = 0; ip < n_rqst_; ++ip) {
        MPI_Request_free(rqst_ + ip);
    }
    for (int ip = 0; ip < n_streams_; ++ip) {
        MPI_Comm_free(thread_comms_ + ip);
        MPIX_Stream_free(streams_ + ip);
    }
    free(rqst_);
    free(streams_);
    free(thread_comms_);
#endif
}
