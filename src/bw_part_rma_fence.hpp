/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef BW_PART_RMA_FENCE_HPP_
#define BW_PART_RMA_FENCE_HPP_
#include "test_part_bw.hpp"

class BwPartRmaFence : public TestPartBw {
    put_info_t* put_info_;
   public:
    BwPartRmaFence() = delete;
    explicit BwPartRmaFence(part_arg_t arg) : TestPartBw(arg) {
        std::tuple<> v;
        m_info(v);
    };

   protected:
    void FileName(const int len, char* filename) override {
        char subname[512];
        TestPartBw::FileName(512, subname);
        snprintf(filename, len, "bw_rma_fence_%s", subname);
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
