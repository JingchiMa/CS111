FILE_ADD=lab2a-add
FILE_LIST=lab2a-list
LOG_FILE_ADD=lab2_add.csv
LOG_FILE_LIST=lab2_list.csv

## checks if two required files exist
if [ ! -f "$FILE_ADD" ]; then
    echo "$FILE_ADD does not exist"
    exit 1
fi
if [ ! -f "$FILE_LIST" ]; then
    echo "$FILE_LIST does not exist"
    exit 1
fi

echo 'running tests for lab2a-add'

# add-none
iterations=(100 1000 10000 100000)
MAX_THREAD=32
for i in "${iterations[@]}"
do
    for thread in $(seq 1 $MAX_THREAD)
    do
        ./"$FILE_ADD" --iterations="$i" --threads="$thread"
    done
done

# add-none-yield
threads=(2 4 8 12)
iterations=(10 20 40 80 100 1000 10000 100000)
for i in "${iterations[@]}"
do
    for thread in "${threads[@]}"
    do
        ./"$FILE_ADD" --iterations="$i" --threads="$thread" --yield
    done
done

# average execution time of yield and unyield
threads=(2 8)
iterations=(100 1000 10000 10000)

for i in "${iterations[@]}"
    do
    for thread in "${threads[@]}"
    do
        ./"$FILE_ADD" --iterations="$i" --threads="$thread"
        ./"$FILE_ADD" --iterations="$i" --threads="$thread" --yield
    done
done

# yield with synchronizations
threads=(2 4 8 12)
iteration=10000
spin_iteration=1000
for thread in "${threads[@]}"
do
    ./"$FILE_ADD" --iterations="$iteration" --threads="$thread" --sync=m --yield
    ./"$FILE_ADD" --iterations="$iteration" --threads="$thread" --sync=c --yield
    ./"$FILE_ADD" --iterations="$spin_iteration" --threads="$thread" --sync=s --yield
done

# large enough number of iterations to overcome issue raised in question 2.1.3
# test all four versions without yield
threads=(2 4 8 12)
iteration=10000
for thread in "${threads[@]}"
do
    ./"$FILE_ADD" --iterations="$iteration" --threads="$thread"
    ./"$FILE_ADD" --iterations="$iteration" --threads="$thread" --sync=m
    ./"$FILE_ADD" --iterations="$iteration" --threads="$thread" --sync=c
    ./"$FILE_ADD" --iterations="$iteration" --threads="$thread" --sync=s
done

echo 'running tests for lab2a-list'

## run program with single thread and increasing number of iterations
thread=1
iterations=(10 100 1000 10000 20000)
for i in "${iterations[@]}"
do
    ./"$FILE_LIST" --iterations="$i" --threads="$thread"
done

## run program to see how many threads and iterations it takes to demonstrate a problem consistently
threads=(2 4 8 12)
iterations=(1 10 100 1000)
for i in "${iterations[@]}"
do
    for thread in "${threads[@]}"
    do
        ./"$FILE_LIST" --iterations="$i" --threads="$thread"
    done
done

## run program with various yield combinations
threads=(2 4 8 12)
iterations=(1 2 4 8 16 32)
yields=("i" "d" "il" "dl")
for i in "${iterations[@]}"
    do
    for thread in "${threads[@]}"
    do
        for yield in "${yields[@]}"
        do
            ./"$FILE_LIST" --iterations="$i" --threads="$thread" --yield="$yield"
        done
    done
done

# demonstrate for moderate number of threads and iterations either synchronization seems to eliminate all problems

thread=12
iteration=(1 2 4 8 16 32)
yields=("i" "d" "il" "dl")
syncs=("s" "m")
for i in "${iterations[@]}"
do
    for yield in "${yields[@]}"
    do
        for sync in "${syncs[@]}"
        do
        ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --yield="$yield"
        ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --yield="$yield" --sync="$sync"
        done
    done
done

## choose an appropriate iteration to overcome start-up costs, and run program without yield for a range of thread numbers

iteration=1000
threads=(1 2 4 8 12 16 24)
syncs=("s" "m")
for thread in "${threads[@]}"
do
    for sync in "${syncs[@]}"
    do
        ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --sync="$sync"
    done
done

