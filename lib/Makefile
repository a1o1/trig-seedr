# lib: host tests, and a ColdFire build check (no undefined symbols: a mod links no libgcc or libc).
#   make test       build and run every test_*.c, each linked with every module
#   make coldfire   compile every module with elekloader's Rytm flags and check `nm -u` is empty
# A new module (*.c) or test (test_*.c) is picked up without editing this file.
CC ?= cc
CROSS ?= m68k-elf-
CFLAGS_HOST = -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
CFLAGS_CF = -mcpu=54455 -O2 -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror

SRC := $(sort $(filter-out test_%.c,$(wildcard *.c)))
HDR := $(wildcard *.h)
TESTS := $(patsubst %.c,%,$(sort $(wildcard test_*.c)))

test: $(TESTS)
	@set -e; for t in $(TESTS); do ./$$t; done

test_%: test_%.c $(SRC) $(HDR)
	$(CC) $(CFLAGS_HOST) $< $(SRC) -o $@

coldfire: $(SRC) $(HDR)
	@set -e; for f in $(SRC); do $(CROSS)gcc $(CFLAGS_CF) -c $$f -o $${f%.c}-cf.o; done
	$(CROSS)ld -r $(SRC:.c=-cf.o) -o lib-cf.o
	$(CROSS)size -t $(SRC:.c=-cf.o)
	@u=$$($(CROSS)nm -u lib-cf.o); if [ -n "$$u" ]; then echo "undefined: $$u"; exit 1; else echo "no undefined symbols"; fi

clean:
	rm -f $(TESTS) *-cf.o lib-cf.o

.PHONY: test coldfire clean
