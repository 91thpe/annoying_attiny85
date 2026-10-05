# Host tests for the AVR-independent code. Run: make -C test
CC ?= cc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -I../src
SRC = test_main.c ../src/rng.c ../src/pattern.c

run: build/test_main
	./build/test_main

build/test_main: $(SRC) ../src/*.h
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -rf build

.PHONY: run clean
