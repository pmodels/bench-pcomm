/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "test_pt2pt_bw.hpp"
#include "tools.hpp"
// run the Bandwdith test
// the test sends n_msg times an increasing number of datatype information.
// the time is started before the send and is stopped after the handshake
// the details of the datatype are set by the heritating class
void TestPt2PtBw::run()
{
    //--------------------------------------------------------------------------
    // get the handshake buffer
    char hdshake_buf[4] = {0};

    // get the BWCommInfo array
    MPI_Request* rqst = (MPI_Request*)malloc(n_msg * sizeof(MPI_Request));

    // setup communications
    SetupComm(n_msg, max_count, &bw_dtype);

    // get the total allocation size
    size_t dtype_size = bw_dtype.alloc_byte;
    m_assert((dtype_size >= 0), "the size should be >= 0 here...");
    m_assert((dtype_size % sizeof(char)) == 0, "the byte count = %ld is not a multiple of sizeof(char) = %ld", dtype_size, sizeof(char));

    m_log("allocation size = %ld * %d * %d = %f MB",dtype_size,n_msg, max_count,n_msg * dtype_size * max_count /1.0e+9);
    char* buf = (char*) malloc(n_msg * dtype_size * max_count);

    //--------------------------------------------------------------------------
    // get the comm size
    int rank;
    int comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    m_assert(comm_size % 2 == 0, "the communicator must be even");

    const bool send  = is_sender(rank, comm_size);
    const int  buddy = get_friend(rank, comm_size);

    //--------------------------------------------------------------------------
    // get the file for output
    char filename[512];
    Filename(512, filename);

    // pre-open the file
    FILE *file;
    if (rank == 0) {
        file = fopen(filename, "w+");
    }

    //--------------------------------------------------------------------------
    // for every count of datatype from 1 to the max count
    for (int count = 1; count <= max_count; count *= 2) {
        double time_acc = 0.0;
        for (int ir = 0; ir < n_repeat; ++ir) {
            size_t buf_count = 0;  // counts the memory in char (the send/recv buffer is in char)
 
            MPI_Barrier(MPI_COMM_WORLD);
            double t0 = MPI_Wtime();
            //----------------------------------------------------------------------
            if (send) {
                // send back-to-back msgs
                for (int is = 0; is < n_msg; ++is) {
                    // get the current datatype info and the corresponding buffer
                    char* lbuf = buf + buf_count;
                    // pre-process the send buffer
                    PreSend(is, count, lbuf);
                    // send the buffer
                    const int mpi_count = count * bw_dtype.dcount;
                    MPI_Isend(lbuf, mpi_count, bw_dtype.dtype, buddy, 100+is, MPI_COMM_WORLD, rqst + is);
                    // increment the memory count
                    buf_count += count * bw_dtype.alloc_byte / sizeof(char);
                }
                MPI_Waitall(n_msg, rqst, MPI_STATUSES_IGNORE);

                // recv the handshake - 4 bytes
                MPI_Recv(hdshake_buf, 4, MPI_CHAR, buddy, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                // send back-to-back msgs
                for (int is = 0; is < n_msg; ++is) {
                    // get the current datatype info and the corresponding buffer
                    char* lbuf = buf + buf_count;
                    // recv in the buffer
                    const int mpi_count = count * bw_dtype.dcount;
                    MPI_Irecv(lbuf, mpi_count, bw_dtype.dtype, buddy, 100+is, MPI_COMM_WORLD, rqst + is);
                    // post-pro the received buffer
                    PostRecv(is, count, lbuf);
                    // increment the memory count
                    buf_count += count * bw_dtype.alloc_byte / sizeof(char);
                }
                MPI_Waitall(n_msg, rqst, MPI_STATUSES_IGNORE);

                // sendthe handshake - 4 bytes
                MPI_Send(hdshake_buf, 4, MPI_CHAR, buddy, 1, MPI_COMM_WORLD);
            }

            //---------------------------------------------------------------------
            time_acc += MPI_Wtime() - t0;
        }
        // get the averaged time among ranks
        double glbl_time;
        MPI_Allreduce(&time_acc, &glbl_time, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        time_acc = glbl_time / (n_repeat * comm_size);

        // get the memory exhanged (which is not the allocated one if the datatype is sparse!)
        size_t mem_dtype = bw_dtype.comm_byte * n_msg;
        const double comm_mem = count * mem_dtype / (1.0e+9);

        // print into the diag file
        if (rank == 0) {
            m_log("%f MB - %f [GB/s]", comm_mem*1e+3, comm_mem / time_acc);
            fprintf(file, "%f,%e\n", comm_mem, comm_mem / time_acc);
        }
    }
    if (rank == 0) {
        fclose(file);
    }

    UpsetComm(max_count,n_msg,&bw_dtype);

    // free stuffs
    free(buf);
    free(rqst);
}
