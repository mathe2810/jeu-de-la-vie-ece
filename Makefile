# Simple Makefile for Jeu de la Vie project (Windows MinGW)
# Targets:
# - projet.exe              : main program (if sources and flags are consistent)
# - motifs_particuliers.exe : pattern analyzer
# - realtime_perf.exe       : performance measurement tool
# - clean                   : remove build artifacts

CC = gcc
CFLAGS = -O2 -Wall

# Output directory
BIN_DIR = bin

# Sources
MAIN_SRC = src/main.c
MOTIFS_SRC = src/motifs_particuliers.c
PERF_SRC = src/realtime_perf.c

# Binaries
MAIN_BIN = $(BIN_DIR)/projet.exe
MOTIFS_BIN = $(BIN_DIR)/motifs_particuliers.exe
PERF_BIN = $(BIN_DIR)/realtime_perf.exe

.PHONY: all clean

all: $(MOTIFS_BIN) $(PERF_BIN)
	@echo Build complete.

$(MOTIFS_BIN): $(MOTIFS_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(PERF_BIN): $(PERF_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Main program (optional, kept here for completeness)
$(MAIN_BIN): $(MAIN_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(MOTIFS_BIN) $(PERF_BIN) $(MAIN_BIN)
