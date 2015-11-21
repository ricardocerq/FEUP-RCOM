CC=gcc
CFLAGS=-O3
BIN=bin

all: transfer1

transfer1: transfer.c applicationLayer.c linkLayer.c
	$(CC) $(CFLAGS) -o transfer transfer.c applicationLayer.c linkLayer.c
