#ifndef VORONOI_CANVAS_H
#define VORONOI_CANVAS_H

#include <stdint.h>
#include <stddef.h>

typedef struct point {
    long x;
    long y;
} point;

typedef struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color;


typedef struct anchor {
    point pos;
    color col;
} anchor;

typedef struct task_arg {
    long start;
    long run;
    point size;
    const anchor *anchors;
    size_t anchors_size;
    color *map;
} task_arg;

point randomPoint(point);
color randomColor(void);
color determinePixelColor(const anchor *, size_t, point);
void *calculateChunk(void *);
int generateVoronoi(color *, point, const anchor *, size_t);
int generatePNG(const char *, const color *, point);
int generateGIF(const char *, anchor *, size_t, color *, point, size_t, int);

#endif
