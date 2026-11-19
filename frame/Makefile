# Host tests for the frame's page tables (the mods are built by elekloader: tools/run_tests.sh).
# make test: test_pages three times, with the library's seed_have_* flags as they are, with every
# feature shown (SEEDR_SHOW_ALL, as the screenshot build) and on the Syntakt (test_pages_st, task P1); test_interplay (task M2.1), the page
# tables and the chain together, also on the Syntakt-shaped profile of lib/props.h (test_interplay_st,
# task S1.2). Binaries go to the untracked build/.
CC ?= cc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
LIB = $(filter-out ../lib/test_%.c, $(wildcard ../lib/*.c))
OUT = ../build

# the Syntakt builds need device/st_params.h, made from your own stock Syntakt file (tools/st_params.sh)
ST_PARAMS := $(shell sh ../tools/st_params.sh)
ST := $(if $(ST_PARAMS),$(OUT)/test_pages_st $(OUT)/test_interplay_st)

test: $(OUT)/test_pages $(OUT)/test_pages_all $(OUT)/test_interplay $(ST)
	$(OUT)/test_pages
	$(OUT)/test_pages_all
	$(OUT)/test_interplay
ifneq ($(ST_PARAMS),)
	$(OUT)/test_pages_st
	@echo "test_interplay (Syntakt profile):"
	$(OUT)/test_interplay_st
else
	@echo "Syntakt builds: skipped (no device/st_params.h; tools/st_params.sh)"
endif

$(OUT)/test_pages: test_pages.c pages.c pages.h $(LIB)
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) test_pages.c pages.c $(LIB) -o $@

$(OUT)/test_pages_all: test_pages.c pages.c pages.h $(LIB)
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) -DSEEDR_SHOW_ALL test_pages.c pages.c $(LIB) -o $@

$(OUT)/test_pages_st: test_pages.c pages.c pages.h $(LIB) ../device/st_params.h
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) -DPAGES_DEVICE_ST test_pages.c pages.c $(LIB) -o $@

$(OUT)/test_interplay: test_interplay.c pages.c pages.h $(LIB) ../lib/props.h
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) test_interplay.c pages.c $(LIB) -o $@

$(OUT)/test_interplay_st: test_interplay.c pages.c pages.h $(LIB) ../lib/props.h ../device/st_params.h
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) -DPROP_DEVICE_ST test_interplay.c pages.c $(LIB) -o $@

clean:
	rm -f $(OUT)/test_pages $(OUT)/test_pages_all $(OUT)/test_pages_st $(OUT)/test_interplay $(OUT)/test_interplay_st

.PHONY: test clean
