#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>

#include "./canvas.h"
#include "./argument.h"

/*
    TODO:
    + verbose argument -v, --versbose
    + help message
    + better error messages
    + create threads only once instead of per frame
    + RLE on color map to reduce memory
    + print frame times
*/

int main(int argc, char **argv) {

    Params options = NEW_PARAMS();
    options = parseArguments(argc, argv);

    long area = options.size.x * options.size.y;

    color *color_map = mmap(NULL, area * sizeof(color), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(color_map == MAP_FAILED) {
        fprintf(stderr, "failed to allocate memory mmap()\n");
        return 1;
    }

    if(options.frames == 1) {
        generateVoronoi(color_map, options.size, options.anchors, options.anchors_size);
        generatePNG(options.filename, color_map, options.size);
    } else {
        generateGIF(options.filename, options.anchors, options.anchors_size, color_map, options.size, options.frames, 3, options.keep);
    }

    if(options.anchors) free(options.anchors);
    if(options.colors) free(options.colors);
    munmap(color_map, area);
    return 0;
}
