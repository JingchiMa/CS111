FILE_LIST=lab2_list
LOG_FILE_LIST=lab2b_list.csv

## checks if the required file exists
if [ ! -f "$FILE_LIST" ]; then
    echo "$FILE_LIST does not exist"
    exit 1
fi

### Part I: results for mutex and spin lock, used for lab2b_1.png
#threads=(1 2 4 8 12 16 24)
#iteration=1000
#syncs=("s" "m")
#for thread in "${threads[@]}"
#do
#    for sync in "${syncs[@]}"
#    do
#        ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --sync="$sync" >> "$LOG_FILE_LIST"
#    done
#done
#
### Part III: timing mutex, used for lab2b_2.png
#threads=(1 2 4 8 12 16 24)
#iteration=1000
#for thread in "${threads[@]}"
#do
#    ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --sync=m >> "$LOG_FILE_LIST"
#done
#
### Part IV: using sublists
#
## yield=id w/o protection
#list=4
#threads=(1 4 8 12 16)
#iterations=(1 2 4 8 16)
#for thread in "${threads[@]}"
#do
#    for i in "${iterations[@]}"
#    do
#        ./"$FILE_LIST" --iterations="$i" --threads="$thread" --yield=id --lists="$list" >> "$LOG_FILE_LIST"
#    done
#done
#
## yield=id w/ protection
#list=4
#threads=(1 4 8 12 16)
#iterations=(10 20 40 80)
#syncs=("s" "m")
#for thread in "${threads[@]}"
#do
#    for i in "${iterations[@]}"
#    do
#        for sync in "${syncs[@]}"
#        do
#            ./"$FILE_LIST" --iterations="$i" --threads="$thread" --yield=id --sync="$sync" --list="$list" >> "$LOG_FILE_LIST"
#        done
#    done
#done

# rerun synchronized versions without yield
iteration=1000
threads=(1 2 4 8 12)
lists=(1 4 8 16)
syncs=("s" "m")

for thread in "${threads[@]}"
do
    for list in "${lists[@]}"
    do
        for sync in "${syncs[@]}"
        do
            ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --sync="$sync" --list="$list" >> "$LOG_FILE_LIST"
        done
    done
done
