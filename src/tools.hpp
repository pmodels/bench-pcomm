/*
 * Copyright (C) by Argonne National Laboratory
 *	See COPYRIGHT in top-level directory
 */
#ifndef TOOLS_HPP_
#define TOOLS_HPP_
#include <cstdio>
#include <cstdlib>
#include <tuple>
#include <mpi.h>


#define m_max(a, b)                                      \
    ({                                                   \
        __typeof__(a) m_max_a_ = (a);                    \
        __typeof__(b) m_max_b_ = (b);                    \
        (m_max_a_ > m_max_b_) ? (m_max_a_) : (m_max_b_); \
    })
#define m_min(a, b)                                      \
    ({                                                   \
        __typeof__(a) m_max_a_ = (a);                    \
        __typeof__(b) m_max_b_ = (b);                    \
        (m_max_a_ > m_max_b_) ? (m_max_b_) : (m_max_a_); \
    })

#ifdef NDEBUG
#define m_assert_def(cond, ...) \
    {                           \
        ((void)0);              \
    }
#else
#define m_assert(cond, ...)                                                                                                        \
    ({                                                                                                                             \
        bool m_assert_cond_ = (bool)(cond);                                                                                        \
        if (!(m_assert_cond_)) {                                                                                                   \
            char m_assert_msg_[1024];                                                                                              \
            int  m_assert_rank_;                                                                                                   \
            MPI_Comm_rank(MPI_COMM_WORLD, &m_assert_rank_);                                                                        \
            snprintf(m_assert_msg_, 1024, __VA_ARGS__);                                                                            \
            fprintf(stdout, "[%d assert] '%s' FAILED: %s (at %s:%d)\n", m_assert_rank_, #cond, m_assert_msg_, __FILE__, __LINE__); \
            fflush(stdout);                                                                                                        \
            MPI_Abort(MPI_COMM_WORLD, MPI_ERR_ASSERT);                                                                             \
        }                                                                                                                          \
    })
#endif

#define m_log(format, ...)                                 \
    ({                                                     \
        char m_log_msg_[1024];                             \
        snprintf(m_log_msg_, 1024, format, ##__VA_ARGS__); \
        fprintf(stdout, "%s\n", m_log_msg_);               \
        fflush(stdout);                                    \
    })

//==============================================================================
typedef struct {
    void*    buf;
    int      count;
    int      target_rank;
    MPI_Win  win;
    MPI_Comm comm;
    MPI_Group group;
} put_info_t;

//==============================================================================
// for debug only
// #include <cxxabi.h>
// template <class T>
// void print_name(T obj) {
//    int status;
//    printf("type => %s\n",
//           abi::__cxa_demangle(typeid(obj).name(), 0, 0, &status));
//}

namespace test {
// this solution is inspired from
// -
// https://stackoverflow.com/questions/9122028/how-to-create-the-cartesian-product-of-a-type-list
// -
// https://github.com/van-Rees-Lab/murphy/blob/develop/test/src/gtest_combine_types.hpp
// and a lot of trial and errors
//--------------------------------------------------------------------------
// merge multiple tuples into a big one
template <typename... T1>
struct merge;
template <class... T>
struct merge<std::tuple<T...>> {
    using type = std::tuple<T...>;
};
template <typename T1, typename T2>
struct merge<T1, T2> {
    using type = std::tuple<T1,T2>;
};
template <class T, class... T1>
struct merge<T, std::tuple<T1...>> {
    using type = std::tuple<T, T1...>;
};
template <class T, class... T1>
struct merge<std::tuple<T1...>, T> {
    using type = std::tuple<T1..., T>;
};
template <class... T1, class... T2>
struct merge<std::tuple<T1...>, std::tuple<T2...>> {
    using type = std::tuple<T1..., T2...>;
};
template <class... T1, class... T2, class... R>
struct merge<std::tuple<T1...>, std::tuple<T2...>, R...> {
    using type =
        typename merge<typename merge<std::tuple<T1...>, std::tuple<T2...>>::type,
                       R...>::type;
};
//--------------------------------------------------------------------------
// combine tuples using the cartesian product
template <typename...>
struct cartprod;
template <class T, class... T2>
struct cartprod<std::tuple<T>, std::tuple<T2...>> {
    using type = std::tuple<typename merge<T, T2>::type...>;
    //using type = std::tuple<typename merge<T, std::tuple<T2>>::type...>;
};
template <class... T1, class... T2>
struct cartprod<std::tuple<T1...>, std::tuple<T2...>> {
    using type = typename merge<
        typename cartprod<std::tuple<T1>, std::tuple<T2...>>::type...>::type;
};
template <class... T1, class... T2, class... R>
struct cartprod<std::tuple<T1...>, std::tuple<T2...>, R...> {
    using type = typename cartprod<
        typename cartprod<std::tuple<T1...>, std::tuple<T2...>>::type,
        R...>::type;
};
//--------------------------------------------------------------------------
// wrap values with types
template <typename T, T value>
struct wrap {
    using type             = T;
    static constexpr T val = value;
};

template <typename T, T... vals>
using values = std::tuple<wrap<T, vals>...>;

//--------------------------------------------------------------------------
// ranges sets
template <int S, int E, int F, typename = void>
struct range;
template <int S, int E, int F>
struct range<S, E, F, std::enable_if_t<(S > E)>> {
    using type = std::tuple<>;
};
template <int S, int E, int F>
struct range<S, E, F, std::enable_if_t<(S <= E)>> {
    using type = typename merge<std::tuple<wrap<int, S>>,
                                typename range<S * F, E, F>::type>::type;
};

//--------------------------------------------------------------------------
// dense sets
template <int S, int E, int F, typename = void>
struct dense;
template <int S, int E, int F>
struct dense<S, E, F, std::enable_if_t<(S > E)>> {
    using type = std::tuple<>;
};
template <int S, int E, int F>
struct dense<S, E, F, std::enable_if_t<(S <= E)>> {
    using type = typename merge<std::tuple<wrap<int, S>>,
                                typename dense<S + F, E, F>::type>::type;
};

////--------------------------------------------------------------------------
//// duplicate
//// the whole list of wrap must be in tuple<tuple<wrap...>>
//template<int N, int V, int I, typename=void>
//struct duplicate;
//template<int N, int V, int I>
//struct duplicate<N,V, I, std::enable_if_t<(N==I)>>{
//    using type = std::tuple<wrap<int,V>>;
//};
//template<int N, int V, int I>
//struct duplicate<N,V,I,std::enable_if_t<(N>I && I > 1)>>{
//    using type = typename merge<std::tuple<wrap<int,V>>,typename duplicate<N,V,I+1>::type>::type;
//};
//template<int N, int V, int I>
//struct duplicate<N,V,I,std::enable_if_t<(N>I && I==1)>>{
//    using type = std::tuple<typename merge<std::tuple<wrap<int,V>>,typename duplicate<N,V,I+1>::type>::type>;
//};

//--------------------------------------------------------------------------
// define the call to the function with a tuple of wrappers or a wrapper
// when we have a random stuff, does nothing
template <typename F, typename T>
void wrap_to_fun(F f, T &t) {}
// when we have a tuple of wrappers
template <typename F, typename... T, int... v>
void wrap_to_fun(F f, std::tuple<wrap<T, v>...> &t) {
    auto t1 = std::make_tuple(v...);
    // print_name(t1);
    (*f)(t1);
}
// when we have only one value -> one wrapper
template <typename F, typename T, int v>
void wrap_to_fun(F f, wrap<T, v> &t) {
    auto t1 = std::make_tuple(v);
    // print_name(t1);
    (*f)(t1);
}

//--------------------------------------------------------------------------
// define the call to the constructor of an object and then to it's function run() with a tuple of wrappers or a wrapper
// when we have a random stuff, does nothing
template <typename O, typename T>
void wrap_to_objnrun(T &t) {}
// when we have a tuple of wrappers
template <typename O, typename... T, int... v>
void wrap_to_objnrun(std::tuple<wrap<T, v>...> &t) {
    auto t1 = std::make_tuple(v...);
    // print_name(t1);
    O obj(t1);
    obj.run();
}
// when we have only one value -> one wrapper
template <typename O, typename T, int v>
void wrap_to_objnrun(wrap<T, v> &t) {
    auto t1 = std::make_tuple(v);
    // print_name(t1);
    O obj(t1);
    obj.run();
}

//--------------------------------------------------------------------------
// define the expansion function
// from
// https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
template <typename F, size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
expand(F f, std::tuple<Tp...> &t) {}
template <typename F, size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), void>::type expand(F f, std::tuple<Tp...> &t) {
    // apply the current item to the function
    wrap_to_fun(f, std::get<I>(t));
    // recrusivelly call the next item
    expand<F, I + 1, Tp...>(f, t);
}

//------------------------------------------------------------------------------
// expand the tuple options and call the wrap_to_objnrun for each of them
template <typename O, size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
expand_objnrun(std::tuple<Tp...> &t) {}
template <typename O, size_t I = 0, typename... Tp>
    inline typename std::enable_if < I<sizeof...(Tp), void>::type expand_objnrun(std::tuple<Tp...> &t) {
    // apply the current item to the function
    wrap_to_objnrun<O>(std::get<I>(t));
    // recrusivelly call the next item
    expand_objnrun<O, I + 1, Tp...>(t);
}

//------------------------------------------------------------------------------
// print a tuple
template <size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
print_tuple(char msg[], std::tuple<Tp...> &t) {}
template <size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), void>::type print_tuple(char msg[], std::tuple<Tp...> &t) {
    // apply the current item to the function
    sprintf(msg, "%s %d", msg, std::get<I>(t));
    // recrusivelly call the next item
    print_tuple<I + 1, Tp...>(msg, t);
}

//--------------------------------------------------------------------------
// enable the return of a subset of a template
// from https://stackoverflow.com/questions/8569567/get-part-of-stdtuple
template <int... list>
struct int_list {
    template <int next>
    struct push_back {
        typedef int_list<list..., next> type;
    };
};
template <size_t max>
struct iota {
    using type = typename iota<max - 1>::type::template push_back<max>::type;
};
template <>
struct iota<0> {
    using type = int_list<>;
};

// for an explanation of the signature, see here:
// https://learn.microsoft.com/en-us/cpp/cpp/decltype-cpp?view=msvc-170
template <typename Tp, int... idx>
constexpr auto idx_get(Tp &t, int_list<idx...>) -> decltype(std::make_tuple(std::get<idx>(t)...)) {
    return std::make_tuple(std::get<idx - 1>(t)...);
}

// returns the id [0 ... I[ of a tuple as a new tuple
template <int I, typename... Tp>
constexpr auto head(std::tuple<Tp...> &t) -> decltype(idx_get(t, typename iota<I>::type())) {
    return idx_get(t, typename iota<I>::type());
}

};  // namespace test

#define m_values(...)    test::values<int, __VA_ARGS__>
#define m_combine(...)   test::cartprod<__VA_ARGS__>::type
#define m_range(...)     test::range<__VA_ARGS__>::type
#define m_dense(...)     test::dense<__VA_ARGS__>::type
//#define m_duplicate(N,V) test::duplicate<N,V,1>::type
#define m_test(O, arg)   test::expand_objnrun<O>(arg);
// #define m_test_fun(func, arg) test::expand(func, arg);

#define m_head(i, t) test::head<i>(t)

// display info about the test
#define m_info(a)                                               \
    ({                                                          \
        int rank;                                               \
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);                   \
        if (rank == 0) {                                        \
            char m_info_msg_[1024];                             \
            snprintf(m_info_msg_, 1024, "TEST %s -", __func__); \
            test::print_tuple(m_info_msg_, a);                  \
            m_log("%s", m_info_msg_);                           \
        }                                                       \
    })

//==============================================================================
static const bool is_sender(const int rank, const int comm_size) {
    const int n_sender = comm_size / 2;
    return (rank < n_sender);
}
static const int get_friend(const int rank, const int comm_size) {
    const int n_sender = comm_size / 2;
    return (rank + n_sender) % comm_size;
}

//==============================================================================

#endif // TOOLS_HPP_
