# lib: host tests, and a ColdFire build check (no undefined symbols: a mod links no libgcc or libc).
#   make test       build and run every test_*.c, each linked with every module; those that use props.h
#                   run a second time on a Syntakt-shaped profile (-DPROP_DEVICE_ST, test_<name>_st)
#   make coldfire   compile every module with elekloader's Rytm flags and check `nm -u` is empty
# A new module (*.c) or test (test_*.c) is picked up without editing this file.
CC ?= cc
CROSS ?= m68k-elf-
CFLAGS_HOST = -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
# -fno-store-merging: m68k-elf-gcc 16.2 at -O2 miscompiles a struct copy followed by stores to its
# fields (one 4-byte store lands 6 bytes early; docs/status/F5.md finding 1). Kept for every ColdFire
# build until the reference compiler is one without the bug.
CFLAGS_CF = -mcpu=54455 -O2 -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror -fno-store-merging

SRC := $(sort $(filter-out test_%.c,$(wildcard *.c)))
HDR := $(wildcard *.h)
TESTS := $(patsubst %.c,%,$(sort $(wildcard test_*.c)))
TESTS_ST := $(patsubst %.c,%_st,$(sort $(shell grep -l '"props.h"' test_*.c)))

test: $(TESTS) $(TESTS_ST)
	@set -e; for t in $(TESTS); do ./$$t; done
	@set -e; for t in $(TESTS_ST); do echo "$$t (Syntakt profile):"; ./$$t; done

test_%_st: test_%.c $(SRC) $(HDR)
	$(CC) $(CFLAGS_HOST) -DPROP_DEVICE_ST $< $(SRC) -o $@

test_%: test_%.c $(SRC) $(HDR)
	$(CC) $(CFLAGS_HOST) $< $(SRC) -o $@

coldfire: $(SRC) $(HDR)
	@set -e; for f in $(SRC); do $(CROSS)gcc $(CFLAGS_CF) -c $$f -o $${f%.c}-cf.o; done
	$(CROSS)ld -r $(SRC:.c=-cf.o) -o lib-cf.o
	$(CROSS)size -t $(SRC:.c=-cf.o)
	@u=$$($(CROSS)nm -u lib-cf.o); if [ -n "$$u" ]; then echo "undefined: $$u"; exit 1; else echo "no undefined symbols"; fi

clean:
	rm -f $(TESTS) $(TESTS_ST) *-cf.o lib-cf.o

.PHONY: test coldfire clean
