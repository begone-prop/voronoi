BIN = voronoi
CC = gcc

CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-variable -std=c99 -pedantic
CLIBS = -lpng -lpthread
IMFLAGS = $(shell pkg-config --cflags --libs MagickWand)

CFILES = argument.c canvas.c voronoi.c
OBJ = argument.o canvas.o voronoi.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS) $(IMFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(CLIBS) $(IMFLAGS)

clean:
	rm -f $(BIN) $(OBJ) frame_*.png
