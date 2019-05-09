#! /usr/bin/gnuplot
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
# lab2b_1.png: throughput(total number of operations per second) for Mutex and Spin-lock

# general plot parameters
set terminal png
set datafile separator ","

# throughput(total number of operations per second) for Mutex and Spin-lock
set title "1: Throughput vs Threads"
set xlabel "Threads"
set logscale x 10
set ylabel "Throughput (operations / ns)"
set logscale y 10
set output 'lab2_list-1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
     "< cat lab2_mutex.csv" using ($2):(1000000000)/($7) \
	title 'mutex' with linespoints lc rgb 'red', \
     "< cat lab2_spinlock.csv" using ($2):(1000000000)/($7) \
	title 'spinlock' with linespoints lc rgb 'green'

