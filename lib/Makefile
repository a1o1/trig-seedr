# gen: host tests, and a ColdFire build check (no undefined symbols: a mod links no libgcc or libc).
#   make test       build and run the host tests
#   make coldfire   compile gen.c and groove.c with elekloader's Rytm flags and check `nm -u` is empty
CC ?= cc
CROSS ?= m68k-elf-
CFLAGS_HOST = -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
CFLAGS_CF = -mcpu=54455 -O2 -ffreestanding -fno-builtin -nostdlib -Wall -Wextra -Werror

test: test_gen test_groove
	./test_gen
	./test_groove

test_gen: gen.c gen.h test_gen.c
	$(CC) $(CFLAGS_HOST) gen.c test_gen.c -o $@

test_groove: gen.c gen.h groove.c groove.h test_groove.c
	$(CC) $(CFLAGS_HOST) gen.c groove.c test_groove.c -o $@

coldfire: gen.c gen.h groove.c groove.h
	$(CROSS)gcc $(CFLAGS_CF) -c gen.c -o gen-cf.o
	$(CROSS)gcc $(CFLAGS_CF) -c groove.c -o groove-cf.o
	$(CROSS)ld -r gen-cf.o groove-cf.o -o lib-cf.o
	$(CROSS)size gen-cf.o groove-cf.o
	@u=$$($(CROSS)nm -u lib-cf.o); if [ -n "$$u" ]; then echo "undefined: $$u"; exit 1; else echo "no undefined symbols"; fi

.PHONY: test coldfire
