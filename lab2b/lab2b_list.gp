#! /usr/bin/gnuplot
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#   8. wait-lock time
#
# output:
# 	1. lab2b_1.png: throughput(total number of operations per second) for Mutex and Spin-lock

# general plot parameters
set terminal png
set datafile separator ","

# throughput(total number of operations per second) for Mutex and Spin-lock
set title "1: Throughput vs Threads"
set xlabel "Threads"
set logscale x 10
set ylabel "Throughput (operations / ns)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'spinlock' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title 'mutex' with linespoints lc rgb 'green'

# wait-for-lock time and average time per operation vs thread num
set title "1: wait-for-lock and average time per operation vs Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "time"
set logscale y 10
set output 'lab2b_2.png'

plot \
"< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
title 'avg time per operation(ns)' with linespoints lc rgb 'red', \
"< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
title 'wait-for-lock time(ns)' with linespoints lc rgb 'green'