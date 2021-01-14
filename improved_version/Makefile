########################################
#
#  CSCI 503 -- Operating Systems
#  Author: Chu-An Tsai
#
########################################

## Compiler, tools and options
CC      = gcc
LINK    = gcc
OPT     = -g

CCFLAGS = $(OPT) -Wall
LDFLAGS = $(OPT)

## Libraries
LIBS = -lpthread
INC  = -I.

## FILES
OBJECTS1 = Tsai_elevator2.o
TARGET1  = Tsai_elevator2



## Implicit rules
.SUFFIXES: .c
.c.o:
	$(CC) -c $(CCFLAGS) $(INC) $<

## Build rules
all: $(TARGET1)

$(TARGET1): $(OBJECTS1)
	$(LINK) -o $@ $(OBJECTS1) $(LDFLAGS) $(LIBS)
clean:
	rm -f $(OBJECTS1) $(TARGET1) 
	rm -f *~ core
