import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import tikzplotlib


#-------------------------------------------------------------------------------
cmap10 = matplotlib.cm.get_cmap('tab10')
cmap20 = matplotlib.cm.get_cmap('tab20')
cmap20b = matplotlib.cm.get_cmap('tab20b')
cmap20c = matplotlib.cm.get_cmap('tab20c')

test_col= dict()
test_col['ompi'] = cmap20c(0)
test_col['mpich'] = cmap20c(4)

#-------------------------------------------------------------------------------
mpilist = ['mpich', 'ompi']
test_list = ['bw_dtype']


save_folder = "/Users/tgillis/git-research/2022_pcomm_notes/figures/results"
save_bool = False

#-------------------------------------------------------------------------------
# run_id = "benchme_2022-11-10-1820-568e_581608"
# bcount_list = [1<<4,1<<5]
# dsize_list = [1<<4]
# stride_list = [1<<4,1<<5,1<<6]

run_id = "benchme_2022-11-10-1823-cf76_581609"
bcount_list = [1<<2,1<<5]
dsize_list = [1<<4]
stride_list = [1<<4,1<<5,1<<6]


#-------------------------------------------------------------------------------

for test in test_list:
    for bcount in bcount_list:
        for dsize in dsize_list:
            for stride in stride_list:
                fig,ax = plt.subplots(num=f"{test}_bcount{bcount}_dsize{dsize}_stride{stride}")
                for mpi in mpilist:
                    filename = f"{run_id}/{mpi}/{test}_bcount{bcount}_dsize{dsize}_stride{stride}.txt"
                    print("opening: "+filename)
                    raw = np.genfromtxt(filename,delimiter=',')
                    msg_size  = raw[:,0]
                    bw = raw[:,1]/1e+3
                   
                    label = f"{mpi}"
                    ax.plot(msg_size,bw,color=test_col[mpi],label=label,linewidth=1.5)
                ax.grid()
                ax.set_xscale('log')
                ax.set_xlabel("msg size [kB]")
                ax.set_ylabel("bandwidth [MB\/s]")
                # add legend after the tikz save
                ax.legend(loc='lower right',ncol=2)


        # if (save_bool):
        #     fig_name = save_folder+f"/{ranks}r_{thread}th_1e-{inoise}" + ".tex"

        #     tikzplotlib.save(fig_name, \
        #         figure=fig, \
        #         extra_axis_parameters=['legend style={at={(0.5,1.1)},anchor=south}'])

        

plt.show()





