FILE_ADD=lab2a-add
FILE_LIST=lab2a-list
LOG_FILE_ADD=lab2_add.csv
LOG_FILE_LIST=lab2_list.csv

## checks if two required files exist
if [ ! -f "$FILE_ADD" ]; then
    echo "$FILE_ADD does not exist"
    exit 1
fi
#if [ ! -f "$FILE_LIST" ]; then
#    echo "$FILE_LIST does not exist"
#    exit 1
#fi

echo 'run tests for lab2a-add'

# add-none 
iterations=(100 1000 10000 100000)
MAX_THREAD=32
for i in "${iterations[@]}"
do 
    for thread in $(seq 1 $MAX_THREAD); 
    do 
        ./"$FILE_ADD" --iterations="$i" --threads="$thread"
    done
done

# add-none-yield
threads=(2 4 8 12)
iterations=(10 20 40 80 100 1000 10000 100000)
for i in "${iterations[@]}"
do 
    for thread in "${threads[@]}";
    do 
        ./"$FILE_ADD" --iterations="$i" --threads="$thread" --yield
    done
done

# average execution time of yield and unyield
threads=(2 8)
iterations=(100 1000 10000 10000)

for i in "${iterations[@]}"
    do
    for thread in "${threads[@]}";
    do
        ./"$FILE_ADD" --iterations="$i" --threads="$thread"
        ./"$FILE_ADD" --iterations="$i" --threads="$thread" --yield
    done
done

# yield with synchronizations
threads=(2 4 8 12)
iterations=10000
spin_iterations=1000
for i in "${iterations[@]}"
do 
    ./"$FILE_ADD" --iterations="$iterations" --threads="$thread" --sync=m --yield
    ./"$FILE_ADD" --iterations="$iterations" --threads="$thread" --sync=c --yield
    ./"$FILE_ADD" --iterations="$spin_iterations" --threads="$thread" --sync=s --yield
done

# large enough number of iterations to overcome issue raised in question 2.1.3
# test all four versions without yield
threads=(2 4 8 12)
iterations=10000
for i in "${iterations[@]}"
do
    ./"$FILE_ADD" --iterations="$iterations" --threads="$thread"
    ./"$FILE_ADD" --iterations="$iterations" --threads="$thread" --sync=m
    ./"$FILE_ADD" --iterations="$iterations" --threads="$thread" --sync=c
    ./"$FILE_ADD" --iterations="$iterations" --threads="$thread" --sync=s
done

