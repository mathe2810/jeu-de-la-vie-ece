# Simple Makefile for Jeu de la Vie project (Windows MinGW)
# Targets:
# - projet.exe              : main program (if sources and flags are consistent)
# - motifs_particuliers.exe : pattern analyzer
# - realtime_perf.exe       : performance measurement tool
# - clean                   : remove build artifacts

CC = gcc
CFLAGS = -O2 -Wall

# Sources
MAIN_SRC = main.c
MOTIFS_SRC = motifs_particuliers.c
PERF_SRC = realtime_perf.c

# Binaries
MAIN_BIN = projet.exe
MOTIFS_BIN = motifs_particuliers.exe
PERF_BIN = realtime_perf.exe

.PHONY: all clean

all: $(MOTIFS_BIN) $(PERF_BIN)
	@echo Build complete.

$(MOTIFS_BIN): $(MOTIFS_SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(PERF_BIN): $(PERF_SRC)
	$(CC) $(CFLAGS) $^ -o $@

# Main program (optional, kept here for completeness)
$(MAIN_BIN): $(MAIN_SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(MOTIFS_BIN) $(PERF_BIN) $(MAIN_BIN)
