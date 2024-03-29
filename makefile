SHELL = /bin/sh

ifeq ($(origin CC),default)
	CC := gcc
	CFLAGS += -Wcast-align=strict
endif
ifeq ($(origin VERSION), undefined)
	VERSION := $(shell git describe --always --dirty --tags)
endif

DEFS := -D _GNU_SOURCE -D VERSION=\"$(VERSION)\" -D ZENITY
CFLAGS := -std=c11 -Wall -Wextra -O3 -g -pedantic -Wformat=2 -Wconversion $(CFLAGS)
FILES := src/main.c src/music.c src/net.c src/utils.c
HEADERS := src/*.h
OUT_EXE := bin/danwmstatus
LIBS := -lpthread -lmpdclient -lX11
# DEBUGFLAGS = -fsanitize=thread
# DEBUGFLAGS = -fsanitize=undefined
# DEBUGFLAGS = -fsanitize=address

.PHONY: all
all: $(OUT_EXE)

$(OUT_EXE): $(FILES) $(HEADERS) | bin
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(DEFS) -o $(OUT_EXE) $(FILES) $(LIBS)

bin:
	mkdir -p bin

.PHONY: clean
clean:
	rm -fr bin/
