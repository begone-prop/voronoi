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
    + output file argument -o, --output, <PATH>    [X]
    + image/gif size argument -s, --size, x | x,y  [X]
    + anchors argument -a, --anchors c | x,y;x,y;...  [X]
    + anchors file argument -A, --anchors_from <PATH>  [X]
    + pallete argument -p, --pallete c | #rrggbb;#rrggbb;...  [X]
    + pallete file argument -P, --pallete_from <PATH>  [X]
    + frames argument -f, --frames c  [X]
    + keep intermediate files argument  -k, --keep  [X]
    + seed -x --seed c  [X]
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
        fprintf(stderr, "failed to allocate memory mmap(): %s\n", strerror(errno));
        return 1;
    }

    if(options.frames == 1) {
        generateVoronoi(color_map, options.size, options.anchors, options.anchors_size);
        generatePNG(options.filename, color_map, options.size);
    } else {
        generateGIF(options.filename, options.anchors, options.anchors_size, color_map, options.size, options.frames, 3, options.keep);
    }

    munmap(color_map, area);
    return 0;
}
