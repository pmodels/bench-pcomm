/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "bw_dtype_3d.hpp"

void BwDtype3D::SetupComm(const int n_msg, const int max_count, BwDtypeInfo* info) {
    m_assert((n_msg * max_count) > 0, "the number of info must be >0");
    // get the datatype, the same for all the msgs
    MPI_Datatype xy_dtype;
    MPI_Datatype xyz_dtype;
    // first datatype is bn[1] blocks of each bn[0] double with a stride of n[0]
    MPI_Type_create_hvector(bn[1], bn[0], n[0] * sizeof(double), MPI_DOUBLE, &xy_dtype);
    // 2nd datatype is bn[2] blocks of each 1 xy_dtypes spaced by n[1]*n[0]
    MPI_Type_create_hvector(bn[2], 1, n[1] * n[0] * sizeof(double), xy_dtype, &xyz_dtype);
    MPI_Type_commit(&xyz_dtype);
    MPI_Type_free(&xy_dtype);

    // setup the info
    // the MPI datatype contains the whole information
    info->dcount     = 1;
    info->dtype      = xyz_dtype;
    info->alloc_byte = n[0] * n[1] * n[2] * sizeof(double);
    info->comm_byte  = bn[0] * bn[1] * bn[2] * sizeof(double);
}
void BwDtype3D::UpsetComm(const int n_msg, const int max_count, BwDtypeInfo* info) {
    m_assert((n_msg*max_count) > 0,"the number of info must be >0");
    MPI_Type_free(&(info[0].dtype));
}
void BwDtype3D::PreSend(const int i_msg, const int count, char* buf) { /*nothing in here*/
}
void BwDtype3D::PostRecv(const int i_msg, const int count, char* buf) { /*nothing in here*/
}

//#include <mpi.h>
//#include "tools.hpp"
//#include "test_list.hpp"
//
//
///**
// * Bandwidth - Datatype - contiguous
// */
//void bw_dtype(bw_dtype_arg &arg){
//    //--------------------------------------------------------------------------
//    m_info(arg);
//    //--------------------------------------------------------------------------
//    const int n_msg = 64;
//    const int repeat = 20;
//    //--------------------------------------------------------------------------
//    // allocate memory
//    const int maxcount = std::get<0>(arg);  // max count: we loop from 1 to the max count to display the bandwidth
//    const int bcount   = std::get<1>(arg);  // data-type block count
//    const int dsize    = std::get<2>(arg);  // data-type block size
//    const int stride   = std::get<3>(arg);  // data-type block stride
//    //m_log("testing with %d dtypes, each of %d blocks of %d doubles, strided at %d",maxcount,bcount,dsize,stride);
//
//    //--------------------------------------------------------------------------
//    // create the datatype
//    MPI_Datatype dtype;
//    MPI_Type_vector(bcount,dsize,stride,MPI_DOUBLE,&dtype);
//    MPI_Type_commit(&dtype);
//    
//    // get the total size and the allocation
//    const size_t ttl_size = maxcount * bcount * stride;
//    double* send_buf =(double*) malloc(sizeof(double) * ttl_size*n_msg);
//    MPI_Request* rqst = (MPI_Request*) malloc(sizeof(MPI_Request) * n_msg);
//
//    //--------------------------------------------------------------------------
//    // get the comm size
//    int rank;
//    int comm_size;
//    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
//    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
//    m_assert(comm_size%2 == 0, "the communicator must be even");
//    
//    const bool send = is_sender(rank,comm_size);
//    const int buddy = get_friend(rank,comm_size);
//
//    //--------------------------------------------------------------------------
//    // get the file for output
//    char filename[512];
//    sprintf(filename,"bw_dtype_bcount%d_dsize%d_stride%d.txt",bcount,dsize,stride);
//    FILE* file;
//    if(rank == 0){
//        file = fopen(filename,"w+");
//    }
//
//    //--------------------------------------------------------------------------
//    // for every count of datatype from 1 to the max count
//    for(int count = 1; count <= maxcount; count*=2){
//        double time_acc = 0.0;
//        for (int ir=0; ir< repeat; ++ir){
//            MPI_Barrier(MPI_COMM_WORLD);
//            double t0 = MPI_Wtime();
//            //----------------------------------------------------------------------
//            if( send ){
//                // send back-to-back msgs
//                for (int is=0; is<n_msg; ++is){
//                    double * buf = send_buf + ttl_size * is;
//                    MPI_Isend(buf,count,dtype,buddy,100,MPI_COMM_WORLD,rqst + is);
//                }
//                MPI_Waitall(n_msg,rqst,MPI_STATUSES_IGNORE);
//
//                // recv the handshake - 4 bytes
//                MPI_Recv(send_buf,4,MPI_CHAR,buddy,101,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
//            }
//            else{
//                // send back-to-back msgs
//                for (int is=0; is<n_msg; ++is){
//                    double * buf = send_buf + ttl_size * is;
//                    MPI_Irecv(buf,count,dtype,buddy,100,MPI_COMM_WORLD,rqst + is);
//                }
//                MPI_Waitall(n_msg,rqst,MPI_STATUSES_IGNORE);
//
//                // sendthe handshake - 4 bytes
//                MPI_Send(send_buf,4,MPI_CHAR,buddy,101,MPI_COMM_WORLD);
//            }
//            
//            //---------------------------------------------------------------------
//            time_acc +=  MPI_Wtime() - t0;
//        }   
//        // get the averaged time among ranks
//        double glbl_time;
//        MPI_Allreduce(&time_acc,&glbl_time,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
//        time_acc = glbl_time / (repeat*comm_size);
//        // get the memory exhanged
//        const double comm_mem = count * bcount * dsize * sizeof(double)/(1.0e+6);
//
//        // print into the diag file
//        if(rank ==0){
//            m_log("%f MB - %e [MB/s]",comm_mem,comm_mem/time_acc);
//            fprintf(file,"%f,%e\n",comm_mem,comm_mem/time_acc);
//        }
//    }
//    if(rank == 0){
//        fclose(file);
//    }
//    
//    // free stuffs
//    MPI_Type_free(&dtype);
//    free(rqst);
//    free(send_buf);
//}
////==============================================================================
