# device: host tests of the device layers against a model of the stock routines, and a ColdFire build check.
#   make test       build and run every test_*.c (rytm_ar2.h with RT_HOST, lib/seed_render.c for seed_step_clear)
#   make coldfire   compile cf_*.c with elekloader's Rytm flags, print its size and check `nm -u` is empty
CC ?= cc
CROSS ?= m68k-elf-
CFLAGS_HOST = -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
# -fno-store-merging: m68k-elf-gcc 16.2 at -O2 miscompiles a struct copy followed by stores to its
# fields (one 4-byte store lands 6 bytes early; docs/status/F5.md finding 1). Kept for every ColdFire
# build until the reference compiler is one without the bug.
CFLAGS_CF = -mcpu=54455 -O2 -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror -fno-store-merging

LIB := $(sort $(filter-out ../lib/test_%.c,$(wildcard ../lib/*.c)))
HDR := $(wildcard *.h) $(wildcard ../lib/*.h)
TESTS := $(patsubst %.c,%,$(sort $(wildcard test_*.c)))
CF := $(sort $(wildcard cf_*.c))

test: $(TESTS)
	@set -e; for t in $(TESTS); do ./$$t; done

test_%: test_%.c $(LIB) $(HDR)
	$(CC) $(CFLAGS_HOST) $< $(LIB) -o $@

coldfire: $(CF) $(HDR)
	@set -e; for f in $(CF); do $(CROSS)gcc $(CFLAGS_CF) -c $$f -o $${f%.c}.o; done
	$(CROSS)size -t $(CF:.c=.o)
	@u=$$($(CROSS)nm -u $(CF:.c=.o)); if [ -n "$$u" ]; then echo "undefined: $$u"; exit 1; else echo "no undefined symbols"; fi

clean:
	rm -f $(TESTS) $(CF:.c=.o)

.PHONY: test coldfire clean
