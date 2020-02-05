VERSION = $(shell git describe)
CC ?= gcc
GNU = -D _GNU_SOURCE
CFLAGS = -std=c11 -Wall -Wextra -O3 -g -pedantic -Wformat=2 -Wconversion -Wcast-align=strict
FILES = src/dwmstatus.c src/music.c src/net.c
OUT_EXE = bin/dwmstatus
LIBS = -lpthread -lmpdclient -lX11
DEBUGFLAGS = -fsanitize=thread
# DEBUGFLAGS = -fsanitize=thread -fsanitize=undefined
# DEBUGFLAGS = -fsanitize=address -fsanitize=undefined

build:
	mkdir -p bin
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(GNU) $(INCLUDES) -D VERSION=\"$(VERSION)\" -o $(OUT_EXE) $(FILES) $(LIBS)

clean:
	rm -f $(OUT_EXE)
