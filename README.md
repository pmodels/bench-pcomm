# Benchmarking for pippelined communication

This code has been used to produce results for

**Quantifying the Performance Benefits of Partitioned Communication in MPI.**
*Thomas Gillis, Ken Raffenetti, Hui Zhou, Yanfei Guo, and Rajeev Thakur* 
In Proceedings of International Conference on Parallel Processing (ICPP23).

## Building the code

- Build `MPICH`` with the options `--enable-threads=multiple --enable-thread-cs=per-vci` and the experimental `--enable-ch4-vci-method=tag` (this last one is optional).
- Create a file in `mark_arch` which describes your specific system:
    - `MPI_DIR` the directory of your MPI installation
    - `CC` and `CXX` the c and c++ compiler
    - `CXXFLAGS` the compilation flags
- Using `MPICH` and for a given number of threads `XX`
```bash
export OMP_PROC_BIND=close
export OMP_PLACES=cores
export OMP_NUM_THREADS=XX
mpiexec -n 2 -l -ppn 1 --bind-to core:XX ./benchme
```


## Licensing

```
Copyright (C) by Argonne National Laboratory
See COPYRIGHT in top-level directory

```


## Additional notes on test logistics

The tests functions can have a `std::tuple` as argument.
It is sometimes convenient to have combinations of parameters. 
For example if the first paramter is either `A` or `B` and the second one is `1` or `2`, then the following combinations are needed:
`A1`, `A2`, `B1`, and `B2`.

We here use the following macros to automate the generation of the parameter space.

To create different values for a given parameters, we have
```c
// a simple set of values
m_values(5,6,9) p1; // gives values 5, 6, 9
// a range set of values with a given start, end, and step: [start, start*step, ..., end]
m_range(1,8,2) p2; // gives the values 1, 2, 4, 8
// a dense set of values with a given start, end, and step: [start, start+step, ... , end]
m_dense(1,8,2) p3; // gives the values 1, 3, 5, 7
```

To combine different paramters and create a set you can use the `m_combine` macros:

```c
m_combine( m_range(1,8,2) , m_dense(1,10,1)) p4;
```

Once your parameter space constructed you can create a test with the object to be run:

```c
m_test(MyObj, p4);
```
This will trigger creation and testing for all the combinations described in `p4`.

:warning: all the parameter space construction is based on variadic recursive templating. 
If the parameter space is not properly constructed the compilation errors might be quite long and hard to figure out.

