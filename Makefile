#!/usr/bin/make -f
OPTALL = -O4 

INCLUDIR = -Iinclude
CC       = gcc $(OPTALL) 
ALL      = xsc xsd 

all:		$(ALL)

xsc:		lz77ind.o lz77outc.o lz77huf.o lz77main.o
		$(CC) -o $@ lz77outc.o lz77ind.o lz77huf.o lz77main.o

xsd:		unlz77.o lz77huf.o
		$(CC) -o $@ unlz77.o lz77huf.o

%.o:		%.c 
		$(CC) -c -o $@ $(INCLUDIR) $<

