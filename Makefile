SRC = voronoi.c
BIN = voronoi
CC = gcc

CFLAGS = -Wall -Wextra -std=c99 -pedantic
CLIBS = -lpng
IMFLAGS = $(shell pkg-config --cflags --libs MagickWand)

all:
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(CLIBS) $(IMFLAGS)

clean:
	rm -f $(BIN) frame_*.png
