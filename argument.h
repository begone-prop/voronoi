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
    .frames = 0, \
    .keep = false, \
    .seed = 0 \
}

static const struct option long_options[] = {
    {"output_file", required_argument, NULL, 'o'},
    {"size", required_argument, NULL, 's'},
    {"anchors", required_argument, NULL, 'a'},
    {"anchors_from", required_argument, NULL, 'A'},
    {"pallete", required_argument, NULL, 'p'},
    {"pallete_from", required_argument, NULL, 'P'},
    {"frames", required_argument, NULL, 'f'},
    {"keep", no_argument, NULL, 'k'},
    {"seed", required_argument, NULL, 'x'},
    {"verbose", optional_argument, NULL, 'v'},
    {"help", optional_argument, NULL, 'h'},
};

Params parseArguments(int, char **);

#endif
