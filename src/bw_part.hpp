/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef BW_PART_HPP_
#define BW_PART_HPP_
#include "mpi.h"
#include "test_part_bw.hpp"

class BwPart : public TestPartBw {

    int n_rqst_;
    MPI_Request rqst_;

   public:
    BwPart() = delete;
    explicit BwPart(part_arg_t arg) : TestPartBw(arg){
        std::tuple<> v;
        m_info(v);
    };

   protected:

    void FileName(const int len, char* filename) override{
        char subname[512];
        TestPartBw::FileName(512,subname);
        snprintf(filename,len,"bw_part_%s",subname);
    }

    void RequestInit(TestPartInfo* info) override;
    void Send_StartPreCompute() override;
    void Send_StartPostCompute() override;
    void Send_Pready(const int i_part) override;
    void Recv_StartPreCompute() override;
    void Recv_StartPostCompute() override;
    void Recv_Pready(const int i_part) override;
    void RequestCleanup(TestPartInfo* info) override;
};

#endif
