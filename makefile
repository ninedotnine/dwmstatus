VERSION = $(shell git describe --always --dirty --tags)
CC ?= gcc
DEFS = -D _GNU_SOURCE -D VERSION=\"$(VERSION)\" -D ZENITY
CFLAGS = -std=c11 -Wall -Wextra -O3 -g -pedantic -Wformat=2 -Wconversion -Wcast-align=strict
FILES = src/main.c src/music.c src/net.c src/utils.c
OUT_EXE = bin/dwmstatus
LIBS = -lpthread -lmpdclient -lX11
DEBUGFLAGS = -D _FORTIFY_SOURCE
# DEBUGFLAGS = -fsanitize=thread
# DEBUGFLAGS = -fsanitize=undefined
# DEBUGFLAGS = -fsanitize=address

build:
	mkdir -p bin
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(DEFS) $(INCLUDES) -o $(OUT_EXE) $(FILES) $(LIBS)

clean:
	rm -f $(OUT_EXE)
