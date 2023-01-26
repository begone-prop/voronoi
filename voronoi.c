#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "./canvas.h"
#include "./argument.h"

/*
    TODO:
    + output file argument -o, --output, <PATH>
    + image/gif size argument -s, --size, x | x,y
    + anchors argument -a, --anchors c | x,y;x,y;...
    + anchors file argument -A, --anchors_from <PATH>
    + pallete argument -p, --pallete c | #rrggbb;#rrggbb;...
    + pallete file argument -P, --pallete_from <PATH>
    + frames argument -f, --frames c
    + keep intermediate files argument  -k, --keep
    + seed -x --seed c
    + verbose argument -v, --versbose
    + help message
    + better error messages
    + create threads only once instead of per frame
    + RLE on color map to reduce memory
    + print frame times
*/

int main(int argc, char **argv) {

    char *output_file = NULL;
    point size = {250, 250};

    size_t def_anchors_size = 10;
    /*anchor *anchors = NULL;*/
    /*size_t anchors_size = 0;*/

    void* addr = malloc(0);
    long seed = (long)*(long*)&addr;
    free(addr);

    srandom(seed);

    Params options = NEW_PARAMS();
    options = parseArguments(argc, argv);

    /*long area = size.x * size.y;*/

    /*const size_t anchors_size = 50;*/
    /*[>const size_t frames = 60;<]*/
    /*[>const int jitter = 5;<]*/

    /*anchor anchors[anchors_size];*/

    /*for(size_t idx = 0; idx < anchors_size; idx++) {*/
        /*anchors[idx].pos = randomPoint(size);*/
        /*anchors[idx].col = randomColor();*/
    /*}*/

    long area = options.size.x * options.size.y;

    color *color_map = mmap(NULL, area * sizeof(color), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(color_map == MAP_FAILED) {
        fprintf(stderr, "failed to allocate memory mmap(): %s\n", strerror(errno));
        return 1;
    }

    options.filename = "./voronoi.png";
    generateVoronoi(color_map, options.size, options.anchors, options.anchors_size);
    generatePNG(options.filename, color_map, options.size);

    /*[>generateGIF(out_gif, anchors, anchors_size, color_map, size, frames, jitter);<]*/
    munmap(color_map, area);
    return 0;
}
