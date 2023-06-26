/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef BW_PACK_3D_HPP_
#define BW_PACK_3D_HPP_
#include "test_pt2pt_bw.hpp"
#include "tools.hpp"

using bw_dtype3d_arg_t = test::merge<bw_arg_t,
                                     std::tuple<int, int> >::type;

class BwPack3D : public TestPt2PtBw {
   private:
    const int bn[3];  // the number of unknowns inside my block in the 3 directions
    const int n[3];   // the total number of unknowns in the 3 directions

    char* usr_buf;  // usr buffer

   public:
    BwPack3D(bw_dtype3d_arg_t arg) : TestPt2PtBw(m_head(bw_arg_s, arg)),
                                     bn{std::get<bw_arg_s + 0>(arg),
                                        std::get<bw_arg_s + 0>(arg),
                                        std::get<bw_arg_s + 0>(arg)},
                                     n{std::get<bw_arg_s + 1>(arg),
                                       std::get<bw_arg_s + 1>(arg),
                                       std::get<bw_arg_s + 1>(arg)} {
        m_info(arg);
        m_assert(n[0] >= bn[0], "you cannot have n = %d < bn = %d", n[0], bn[0]);
        m_assert(n[1] >= bn[1], "you cannot have n = %d < bn = %d", n[1], bn[1]);
        m_assert(n[2] >= bn[2], "you cannot have n = %d < bn = %d", n[2], bn[2]);
    };

    virtual void Filename(const int len, char* name) override {
        snprintf(name, len, "bw_pack3d_n%d-%d-%d_bn%d-%d-%d.txt", n[0], n[1], n[2], bn[0], bn[1], bn[2]);
    }

    virtual void SetupComm(const int n_msg, const int max_count, BwDtypeInfo* info) override;
    virtual void UpsetComm(const int n_msg, const int max_count, BwDtypeInfo* info) override;
    virtual void PreSend(const int i_msg, const int count, char* buf) override;
    virtual void PostRecv(const int i_msg, const int count, char* buf) override;
};

#endif
