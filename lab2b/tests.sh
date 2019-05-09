FILE_LIST=lab2_list
LOG_FILE_LIST=lab2b_list.csv

## checks if the required file exists
if [ ! -f "$FILE_LIST" ]; then
    echo "$FILE_LIST does not exist"
    exit 1
fi

# results for mutex and spin lock, used for lab2b_1.png
threads=(1 2 4 8 12 16 24)
iteration=1000
syncs=("s" "m")
for thread in "${threads[@]}"
do
    for sync in "${syncs[@]}"
    do
        ./"$FILE_LIST" --iterations="$iteration" --threads="$thread" --sync="$sync" >> "$LOG_FILE_LIST"
    done
done
