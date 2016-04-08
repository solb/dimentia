#       
# Makefile for building a program that calls a routine from
# the CLAPACK Library
#
# Dr. Andrew J. Pounds
# Departments of Chemistry and Computer Science
# Mercer University
#
# November 2004
#

#svd:
#	$(CC) svd.cpp -o svd -lblas -llapack

all: dgesv svd

CC=clang
LDFLAGS=-lblas -llapack
CFLAGS=

dgesv: dgesv.cpp
	$(CC) $< -o $@ $(LDFLAGS) $(CFLAGS)


svd: svd.cpp
	$(CC) $< -o $@ $(LDFLAGS) $(CFLAGS)

