## NAME:Jingchi Ma
## EMAIL:majc@g.ucla.edu
## ID:705027270

TARGET = lab2_list
CC = gcc
CFLAGS = -g -Wall -Wextra -lpthread -lrt -lprofiler

.PHONY: clean all default tests graphs

default: all
all: $(TARGET);

SOURCES = $(wildcard *.c) 
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
HEADERS = $(wildcard *.h)

DUMPFILE = ./raw.gperf

DISTFILES = $(wildcard *.c) $(HEADERS)  Makefile README tests.sh profile.sh profile.out

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

tests:
	chmod +x tests.sh
	-./tests.sh

graphs:
	chmod +x lab2b_list.gp
	./lab2b_list.gp

profile:
	chmod +x profile.sh
	./profile.sh	
clean: 
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f *.tar.gz
	#-rm -f *.png
	# -rm -f *.csv

dist:
	chmod +x prepare_dependencies.sh
	-./prepare_dependencies.sh # > /dev/null 2>&1 
	tar cvzf lab2a-705027270.tar.gz $(DISTFILES)
