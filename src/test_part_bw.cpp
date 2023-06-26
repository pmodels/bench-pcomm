/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#include "test_part_bw.hpp"
#include "mpi.h"
#include "tools.hpp"
#include "test_part_bw.hpp"
#include <random>
#include <omp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <climits>
#include <map>
#include <atomic>

#define MAX_RERUN 50
#define THRESHOLD_RERUN 0.05
#define HANDSHAKE_TAG 0

using std::map;
static constexpr int    upper_rank = 1000;  // approximates the infinity of procs
static map<int, double> t_nu       = {{0, 0.0},
                                      {1, 6.314},
                                      {2, 2.920},
                                      {3, 2.353},
                                      {4, 2.132},
                                      {5, 2.015},
                                      {7, 1.895},
                                      {10, 1.812},
                                      {15, 1.753},
                                      {20, 1.725},
                                      {30, 1.697},
                                      {50, 1.676},
                                      {100, 1.660},
                                      {upper_rank, 1.645}};
/**
 * @brief return the t_nu for 90% confidence interval width based on the interpolation of the above table
 *
 * @param nu the number of proc-1
 * @return double the confidence interval param
 */
double t_nu_interp(const int nu) {
    m_assert(nu >= 0, "the nu param = %d must be positive", nu);
    //--------------------------------------------------------------------------
    if (nu == 0) {
        // easy, it's 0
        return 0.0;
    } else if (nu <= 5) {
        // we have an exact entry
        const auto it = t_nu.find(nu);
        return it->second;
    } else if (nu >= upper_rank) {
        // we are too big, it's like a normal distribution
        return 1.645;
    } else {
        // find the right point
        auto         it_up  = t_nu.lower_bound(nu);  // first element >= nu
        const int    nu_up  = it_up->first;
        const double t_up   = it_up->second;
        auto         it_low = std::prev(it_up, 1);  // take the previous one
        const int    nu_low = it_low->first;
        const double t_low  = it_low->second;
        // m_log("nu_up = %d, nu_low = %d, t_up=%f, t_low=%f",nu_up,nu_low,t_up, t_low);
        return t_low + (t_up - t_low) / (nu_up - nu_low) * (nu - nu_low);
    }
    //--------------------------------------------------------------------------
}

/* returns the noices (in permil)*/
void TestPartBw::get_noise_(const int n_noise, double* noise_permil) {
    //--------------------------------------------------------------------------
    //  GET RANDOM NUMBERS GENERATOR
    //--------------------------------------------------------------------------
    std::random_device noise_rd{};
    std::mt19937       noise_gen{noise_rd()};

    // on average there are no noise but the std is given as a percentage
    const double noise_mean = 0.0;  // on average no noise
    const double noise_std  = 0.001 * noise_lvl;

    // get a normal distributed noise
    std::normal_distribution<double> noise{noise_mean, noise_std};

    // fill the buffer with random percentages
    for (int it = 0; it < n_noise; ++it) {
        // no negative number here
        noise_permil[it] = fabs(noise(noise_gen));
    }
}

void TestPartBw::run() {
    // impose the number of threads
    omp_set_num_threads(n_threads);

    // get the communicator and abor if it's not what we expect
    int       rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    if ((comm_size % 2) != 0 || comm_size < 2) {
        printf("COMM_WORLD size must be even and >2: now %d", comm_size);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    const int buddy = get_friend(rank,comm_size);

    //--------------------------------------------------------------------------
    char foldr_name [512] = "results";
    char fullname[512];

    // pre-open the file to clean it up
    if (rank == 0) {
        char filename[512];
        FileName(512, filename);
        snprintf(fullname, 512, "%s/%s", foldr_name, filename);

        // test and create the dir if needed
        struct stat st = {0};
        if (rank == 0 && stat(foldr_name, &st) == -1) {
            mkdir(foldr_name, 0770);
        }
        FILE* file;
        file = fopen(fullname, "w+");
        fclose(file);
    }

    // the minimum total size is n_part as it means 1 element per partition
    for (size_t test_size = n_part; test_size <= max_count; test_size *= 2) {
        // get the sizes
        TestPartInfo info;
        info.size.per_rank  = test_size;
        info.size.per_part  = test_size / n_part;
        info.size.bandwidth = test_size * sizeof(double*);

        // allocate the send buffer and the noise
        info.buf = (double*)malloc(info.size.per_rank * sizeof(double));
        // init the communication
        RequestInit(&info);

        // get the overhead of MPI_Wtime() and decide to apply some noise or not
        const double threshold_start = MPI_Wtime();
        for (int i = 0; i < 1000; ++i) {
            MPI_Wtime();
        }
        const double threshold_end   = MPI_Wtime();
        const double noise_threshold = (threshold_end - threshold_start) / (1000 + 2);
        // we iterate on the MPI_Wtime if calling 4 times Wtime() is faster than the time we have to
        // wait. the factor 4 comes from the number of times we have to call the function to
        // successfully measure the sleep
        const double noise_time = 1.0e-12 * noise_lvl * (info.size.per_part * sizeof(double));
        const bool   do_noise   = (noise_time > (3.0 * noise_threshold));

        //----------------------------------------------------------------------
        //  SEND - RECV
        //----------------------------------------------------------------------
        const bool sender = is_sender(rank, comm_size);
        int        rerun  = 0;
        do {
            double* t0_data      = (double*)calloc((n_repeat + n_warmup), sizeof(double));
            double* t0_cmpt_data = (double*)calloc((n_repeat + n_warmup), sizeof(double));

            for (int iter = 0; iter < (n_repeat + n_warmup); ++iter) {
                // iteration specific timers
                double t0      = 0.0;
                double t0_cmpt = 0.0;

                MPI_Barrier(MPI_COMM_WORLD);
                //======================================================================================
                // BEGIN PARALLEL REGION
                //======================================================================================
#pragma omp parallel reduction(max : t0, t0_cmpt)
                {
                    double t0_tic   = 0.0;
                    double t0_toc  = 0.0;
                    double cmpt_tic = 0.0;
                    double cmpt_toc = 0.0;
                    //..................................................................
#pragma omp barrier
                    t0_tic = MPI_Wtime();
                    //..................................................................
                    // SEND
                    if (sender) {
                        Send_StartPreCompute();
                        // NO BARRIER - done in Send_StartPreCompute
#pragma omp for schedule(static) nowait
                        for (int ip = 0; ip < n_part; ip++) {
                            // only the last partition sleeps to get the early bird behavior
                            if (ip == n_part - 1 && do_noise) {
                                cmpt_tic = MPI_Wtime();
                                while ((MPI_Wtime() - cmpt_tic) < noise_time) {
                                    // do nothing
                                };
                                cmpt_toc = MPI_Wtime();
                            }
                            // partition is ready
                            Send_Pready(ip);
                        }
                        // NO BARRIER - done in Send_StartPostCompute
                        // finalize
                        Send_StartPostCompute();
                    }
                    //..................................................................
                    // RECV
                    if (!sender) {
                        Recv_StartPreCompute();
                        // NO BARRIER - done in Send_StartPreCompute
#pragma omp for schedule(static) nowait
                        for (int ip = 0; ip < n_part; ip++) {
                            Recv_Pready(ip);
                        }
                        // NO BARRIER - done in Send_StartPreCompute
                        Recv_StartPostCompute();
                    }
                    //..................................................................
                    t0_toc = MPI_Wtime();
#pragma omp barrier
                    t0      = m_max(t0_toc - t0_tic, t0);
                    t0_cmpt = m_max(cmpt_toc - cmpt_tic, t0_cmpt);
                    //..................................................................
                }
                //======================================================================================
                // END PARALLEL REGION
                //======================================================================================
                MPI_Barrier(MPI_COMM_WORLD);
                // store the timings
                t0_data[iter]      = t0;
                t0_cmpt_data[iter] = t0_cmpt;
            }
            //..........................................................................................
            // local timers and local stds
            double t_zero_local = 0.0;
            double t_cmpt_local = 0.0;
            // first get the means
            for (int i = n_warmup; i < (n_repeat + n_warmup); ++i) {
                t_zero_local += t0_data[i];
                t_cmpt_local += t0_cmpt_data[i];
            }
            t_zero_local /= n_repeat;
            t_cmpt_local /= n_repeat;
            // then the std
            double s_zero_local = 0.0;
            double s_cmpt_local = 0.0;
            for (int i = n_warmup; i < (n_repeat + n_warmup); ++i) {
                s_zero_local += pow(t0_data[i] - t_zero_local, 2);
                s_cmpt_local += pow(t0_cmpt_data[i] - t_cmpt_local, 2);
            }
            s_zero_local = sqrt(s_zero_local / (n_repeat - 1));
            s_cmpt_local = sqrt(s_cmpt_local / (n_repeat - 1));
            // free useless data
            free(t0_data);
            free(t0_cmpt_data);
            //..................................................................
            // get different comms for the sender and receivers
            int      color = is_sender(rank, comm_size);
            MPI_Comm new_comm;
            MPI_Comm_split(MPI_COMM_WORLD, color, rank, &new_comm);
            int new_comm_size;
            MPI_Comm_size(new_comm, &new_comm_size);

            double t_zero, t_cmpt, std_zero, std_cmpt;
            MPI_Reduce(&t_zero_local, &t_zero, 1, MPI_DOUBLE, MPI_SUM, 0, new_comm);
            MPI_Reduce(&s_zero_local, &std_zero, 1, MPI_DOUBLE, MPI_SUM, 0, new_comm);
            MPI_Reduce(&t_cmpt_local, &t_cmpt, 1, MPI_DOUBLE, MPI_SUM, 0, new_comm);
            MPI_Reduce(&s_cmpt_local, &std_cmpt, 1, MPI_DOUBLE, MPI_SUM, 0, new_comm);
            t_zero /= (new_comm_size);
            t_cmpt /= (new_comm_size);
            std_zero /= (new_comm_size);
            std_cmpt /= (new_comm_size);
            MPI_Comm_free(&new_comm);

            //..................................................................
            if (rank == 0) {
                double t_recv, std_recv;
                MPI_Recv(&t_recv, 1, MPI_DOUBLE, 1, 404, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&std_recv, 1, MPI_DOUBLE, 1, 405, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // get the memory in B
                const size_t memory = info.size.bandwidth;

                // get the individual 90% confidence intervals (CI)
                const double t_nu_val = t_nu_interp(n_repeat);
                const double ci_zero  = std_zero * t_nu_val * sqrt(1.0 / n_repeat);
                const double ci_recv  = std_recv * t_nu_val * sqrt(1.0 / n_repeat);
                const double ci_cmpt  = std_cmpt * t_nu_val * sqrt(1.0 / n_repeat);
                // get the CI for the difference of two means following:
                // https://sphweb.bumc.bu.edu/otlt/mph-modules/bs/bs704_confidence_intervals/bs704_confidence_intervals5.html
                // with n1 = n2 = n_repeat
                const double time      = t_recv - t_cmpt;
                const double t_nu_diff = t_nu_interp(2 * n_repeat - 2);
                const double s_p       = sqrt(0.5 * (pow(std_recv, 2) + pow(std_cmpt, 2)));
                const double ci_time   = t_nu_diff * s_p * sqrt(2.0 / n_repeat);

                // we are a sender
                const double bw = ((double)memory / 1.0e+9) / (time);

                if (ci_time / time > THRESHOLD_RERUN && rerun < (MAX_RERUN-1)) {
                    rerun++;
                    MPI_Send(&rerun, 1, MPI_INT, 1, 406, MPI_COMM_WORLD);
                    m_log("\t%f KB - %.2f +- %.2f [usec]- %f [GB/s] -> (%.2f%%) retry %d/%d", (double)memory * 1.0e-3, time * 1e+6, ci_time * 1e+6, bw,ci_time/time*100.0,rerun,MAX_RERUN);
                } else {
                    rerun = (MAX_RERUN * 2);
                    MPI_Send(&rerun, 1, MPI_INT, 1, 406, MPI_COMM_WORLD);
                    m_log("%f KB - %.2f +- %.2f [usec]- %f [GB/s] - send = %e, recv = %e -> (%.2f%%)", (double)memory * 1.0e-3, time * 1e+6, ci_time * 1e+6,bw,(t_zero-t_cmpt)*1e+6,(t_recv-t_cmpt)*1e+6, ci_time/time*100.0);
                    //  print to file
                    FILE* file;
                    file = fopen(fullname, "a+");
                    fprintf(file, "%ld,%e,%e,%e,%e,%e,%e,%e,%e\n", memory, t_zero, t_recv, t_cmpt, time, ci_zero, ci_recv, ci_cmpt, ci_time);
                    fclose(file);
                }
            } else {
                // send the receive time to the sender
                MPI_Send(&t_zero, 1, MPI_DOUBLE, 0, 404, MPI_COMM_WORLD);
                MPI_Send(&std_zero, 1, MPI_DOUBLE, 0, 405, MPI_COMM_WORLD);

                MPI_Recv(&rerun, 1, MPI_INT, 0, 406, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            // decide if we want to rerun the simulation based on the 90%-CI
        } while (rerun <= MAX_RERUN);
        //..................................................................
        RequestCleanup(&info);
        free(info.buf);
    }
}

