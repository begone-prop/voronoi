BIN = voronoi
CC = gcc

CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=c99 -pedantic
CLIBS = -lpng
IMFLAGS = $(shell pkg-config --cflags --libs MagickWand)

CFILES = argument.c canvas.c voronoi.c
OBJ = argument.o canvas.o voronoi.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(CLIBS) $(IMFLAGS)

clean:
	rm -f $(BIN) $(OBJ) frame_*.png
