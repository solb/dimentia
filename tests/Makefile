LLVERSION := 3.7

CC := clang-$(LLVERSION)
CLEAN := git clean
OPT := opt-$(LLVERSION)

CFLAGS := -std=c99 -g -O0 -Wall -Wextra -Wpedantic

########

C_SOURCES := $(wildcard *.c)

.PHONY: all
all: $(C_SOURCES:.c=.ll)

.PHONY: help
help:
	@echo "USAGE: make testprog    - ELF executable"
	@echo "       make testprog.BC - Unoptimized LLVM bitcode"
	@echo "       make testprog.bc - mem2reg'd LLVM bitcode"
	@echo "       make testprog.ull - Unoptimized LLVM IR assembly"
	@echo "       make testprog.ll - mem2reg'd LLVM IR assembly"
	@echo "       make testprog.s  - Assembly code"
	@echo "       make testprog.o  - Object file"
	@echo "       make clean       - Remove all generated files"

.PHONY: clean
clean:
	$(CLEAN) -fX

.SECONDARY:

%.BC: %.c
	$(CC) -c -emit-llvm $(CPPFLAGS) $(CFLAGS) $< -o $@

%.bc: %.BC
	$(OPT) -mem2reg $< -o $@

%.ull: %.c
	$(CC) -S -emit-llvm $(CPPFLAGS) $(CFLAGS) $< -o $@

%.ll: %.ull
	$(OPT) -S -mem2reg $< -o $@

%.s: %.c
	$(CC) -S $(CPPFLAGS) $(CFLAGS) $<
