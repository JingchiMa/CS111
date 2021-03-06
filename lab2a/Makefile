## NAME:Jingchi Ma
## EMAIL:majc@g.ucla.edu
## ID:705027270

TARGET_1 = lab2_add
TARGET_2 = lab2_list
CC = gcc
CFLAGS = -g -Wall -Wextra -lpthread -lrt

.PHONY: clean all default tests graphs

default: all
all: $(TARGET_1) $(TARGET_2);

SOURCES_1 = lab2_add.c # lab2a-add
SOURCES_2 = $(filter-out lab2_add.c, $(wildcard *.c)) # lab2a-list
OBJECTS_1 = $(patsubst %.c, %.o, $(SOURCES_1))
OBJECTS_2 = $(patsubst %.c, %.o, $(SOURCES_2))
HEADERS = $(wildcard *.h)

DISTFILES = $(wildcard *.c) $(HEADERS)  Makefile README tests.sh prepare_dependencies.sh lab2_add.gp lab2_list.gp lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png

# DISTFILES = $(wildcard *.c) $(HEADERS)  Makefile README tests.sh prepare_dependencies.sh lab2_add.gp lab2_list.gp $(wildcard *.png) $(wildcard *.csv)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_1): $(OBJECTS_1)
	$(CC) $(CFLAGS) $(OBJECTS_1) -o $@

$(TARGET_2): $(OBJECTS_2)
	$(CC) $(CFLAGS) $(OBJECTS_2) -o $@

tests:
	chmod +x tests.sh
	-./tests.sh

graphs:
	./lab2_add.gp 
	./lab2_list.gp
clean: 
	-rm -f *.o
	-rm -f $(TARGET_1)
	-rm -f $(TARGET_2)
	-rm -f *.tar.gz
	#-rm -f *.png
	# -rm -f *.csv

dist:
	chmod +x prepare_dependencies.sh
	-./prepare_dependencies.sh # > /dev/null 2>&1 
	tar cvzf lab2a-705027270.tar.gz $(DISTFILES)
