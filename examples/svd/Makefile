all: dgesv svd

CC=clang++
LDFLAGS=-lblas -llapack
CFLAGS=-std=c++11

dgesv: dgesv.cpp
	$(CC) $< -o $@ $(LDFLAGS) $(CFLAGS)


svd: svd.cpp
	$(CC) $< -o $@ $(LDFLAGS) $(CFLAGS)

