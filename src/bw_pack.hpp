/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef BW_PACK_HPP_
#define BW_PACK_HPP_

#include "tools.hpp"
#include "test_pt2pt_bw.hpp"

using bw_pack_arg_t = test::merge<bw_arg_t,
                                   std::tuple<int, int, int> >::type;

class BwPack: public TestPt2PtBw{
   private:
    const int bcount;   // the number of blocks within a data-type
    const int bsize;    // the size of a block
    const int bstride;  // the stride of a block

    char* usr_buf;

   public:
    BwPack(bw_pack_arg_t arg) : TestPt2PtBw(m_head(bw_arg_s, arg)),
                                bcount{std::get<bw_arg_s + 0>(arg)},
                                bsize{std::get<bw_arg_s + 1>(arg)},
                                bstride{std::get<bw_arg_s + 2>(arg)} {
        m_info(arg);
        m_assert(bstride >= bsize, "you cannot have a strice = %d < size = %d", bstride, bsize);
    };

    virtual void Filename(const int len, char* name) override {
        snprintf(name, len, "bw_pack_bcount%d_bsize%d_bstride%d.txt", bcount, bsize, bstride);
    }

    virtual void SetupComm(const int max_count, const int n_info, BwDtypeInfo* info) override;
    virtual void UpsetComm(const int max_count, const int n_info, BwDtypeInfo* info) override;
    virtual void PreSend(const int i_msg, const int count, char* buf) override;
    virtual void PostRecv(const int i_msg, const int count, char* buf) override;
};
#endif
