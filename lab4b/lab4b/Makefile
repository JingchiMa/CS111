## NAME:Jingchi Ma
## EMAIL:majc@g.ucla.edu
## ID:705027270

TARGET = lab4b
CC = gcc
CFLAGS = -g -Wall -Wextra 

.PHONY: clean all default

default: all
all: $(TARGET) 

SOURCES = $(wildcard *.c) 
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
HEADERS = $(wildcard *.h)

DISTFILES = $(wildcard *.c) $(HEADERS)  Makefile README

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

clean: 
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f *.tar.gz

check:
	

dist:
	tar cvzf lab4b-705027270.tar.gz $(DISTFILES)

