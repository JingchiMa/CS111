FILE_ADD=lab2_add
FILE_LIST=lab2_list
LOG_FILE_ADD=lab2_add.csv
LOG_FILE_LIST=lab2_list.csv
## checks if two required files exist
if [ ! -f "$FILE_ADD" ] || [ ! -f "$FILE_LIST" ]; then
	make
fi

if [ ! -f "$LOG_FILE_ADD" ] || [ ! -f "$LOG_FILE_LIST" ]; then
        make tests
fi

DIAGRAM_FILE=lab2_add-1.png
if [ ! -f "$DIAGRAM_FILE" ]; then
	make graphs
fi
exit 0
