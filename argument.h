#ifndef VORONOI_ARGUMENT_H
#define VORONOI_ARGUMENT_H

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include "./canvas.h"

typedef struct Params {
    const char *filename;
    point size;
    anchor *anchors;
    size_t anchors_size;
    color *colors;
    size_t colors_size;
    int frames;
    bool keep;
    long seed;
} Params;

#define NEW_PARAMS() (Params){ \
    .filename = NULL, \
    .size = {250, 250}, \
    .anchors = NULL, \
    .anchors_size = 5, \
    .colors = NULL, \
    .colors_size = 60, \
    .frames = 1, \
    .keep = false, \
    .seed = 0 \
}

Params parseArguments(int, char **);

#endif
