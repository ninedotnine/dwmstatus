ifeq ($(origin CC),default)
	CC := gcc
	CFLAGS += -Wcast-align=strict
endif
VERSION := $(shell git describe --always --dirty --tags)
DEFS := -D _GNU_SOURCE -D VERSION=\"$(VERSION)\" -D ZENITY
CFLAGS += -std=c11 -Wall -Wextra -O3 -g -pedantic -Wformat=2 -Wconversion
FILES := src/main.c src/music.c src/net.c src/utils.c
HEADERS := src/*.h
OUT_EXE := bin/dwmstatus
LIBS := -lpthread -lmpdclient -lX11
DEBUGFLAGS := -D _FORTIFY_SOURCE
# DEBUGFLAGS = -fsanitize=thread
# DEBUGFLAGS = -fsanitize=undefined
# DEBUGFLAGS = -fsanitize=address

.PHONY: all
all: $(OUT_EXE)

$(OUT_EXE): $(FILES) $(HEADERS) | bin
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(DEFS) $(INCLUDES) -o $(OUT_EXE) $(FILES) $(LIBS)

bin:
	mkdir -p bin

.PHONY: clean
clean:
	rm -fr bin/
