#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <err.h>

#include "./canvas.h"
#include "./argument.h"

/*
    TODO:
    + verbose argument -v, --versbose
    + help message
    + create threads only once instead of per frame
    + RLE on color map to reduce memory
    + print frame times
    + create intermediate images in /tmp
*/

int main(int argc, char **argv) {

    Params options = NEW_PARAMS();
    options = parseArguments(argc, argv);

    long area = options.size.x * options.size.y * sizeof(color);

    color *color_map = mmap(NULL, area, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(color_map == MAP_FAILED) {
        err(1, "mmap()");
    }

    if(options.frames == 1) {
        if(generateVoronoi(color_map, options.size, options.anchors, options.anchors_size) == 0) {
            errx(1, "Exiting ...");
        }

        if(generatePNG(options.filename, color_map, options.size) == 0) {
            errx(1, "Exiting ...");
        }
    } else {
        if(generateGIF(options.filename, options.anchors, options.anchors_size, color_map, options.size, options.frames, 3, options.keep) == 0) {
            errx(1, "Exiting ...");
        }
    }

    if(options.anchors) free(options.anchors);
    if(options.colors) free(options.colors);
    munmap(color_map, area);
    return 0;
}
