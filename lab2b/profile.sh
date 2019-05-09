rm profile.out
rm raw.gperf
LDPRELOAD=/usr/lib64/libprofiler.so 
CPUPROFILE=raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
pprof -text lab2_list raw.gperf > profile.out 
pprof -list=acquire_spin_lock lab2_list raw.gperf >> profile.out
