CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -g -pedantic -Wconversion -Wcast-align=strict
FILES = src/dwmstatus.c
OUT_EXE = bin/dwmstatus
LIBS = -lpthread -lmpdclient -lX11
DEBUGFLAGS = -fsanitize=thread -fsanitize=undefined
# DEBUGFLAGS = -fsanitize=address -fsanitize=undefined


build:
	mkdir -p bin
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(INCLUDES) -o $(OUT_EXE) $(FILES) $(LIBS)

clean:
	rm -f $(OUT_EXE)
