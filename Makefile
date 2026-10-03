# Makefile for Washing Machine Control Unit (CO3053 - BTL 2)

CC = gcc
CFLAGS ?= -Wall -Wextra -Werror -O2 -I src/include -I src/hal

SRC = src/fsm/washing_machine_fsm.c src/hal/mock_hal.c

.PHONY: all test sim clean

all: test sim

test: $(SRC) tests/test_washing_machine.c
	$(CC) $(CFLAGS) $^ -o test_runner.exe
	./test_runner.exe

sim: $(SRC) sim/sim_interactive.c
	$(CC) $(CFLAGS) $^ -o sim_wm.exe

clean:
	rm -f test_runner.exe sim_wm.exe *.o
