# Host tests for the mod's page tables (the mod itself is built by elekloader, tools/test_seedr.sh).
# make test: test_pages twice, with the library's seed_have_* flags as they are and with every
# feature shown (SEEDR_SHOW_ALL, as the screenshot build). Binaries go to the untracked build/.
CC ?= cc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
LIB = $(filter-out ../../lib/test_%.c ../../lib/gen.c ../../lib/groove.c, $(wildcard ../../lib/*.c))
OUT = ../../build

test: $(OUT)/test_pages $(OUT)/test_pages_all
	$(OUT)/test_pages
	$(OUT)/test_pages_all

$(OUT)/test_pages: test_pages.c pages.c pages.h $(LIB)
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) test_pages.c pages.c $(LIB) -o $@

$(OUT)/test_pages_all: test_pages.c pages.c pages.h $(LIB)
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) -DSEEDR_SHOW_ALL test_pages.c pages.c $(LIB) -o $@

clean:
	rm -f $(OUT)/test_pages $(OUT)/test_pages_all

.PHONY: test clean
