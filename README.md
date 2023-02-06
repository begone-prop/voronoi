# A multithreaded Voronoi diagram generator

## Description

The following program generates an image or GIF file of a [Voronoi diagram](https://en.wikipedia.org/wiki/Voronoi_diagram)
according to the parameters specified as arguments on the command line.
It does this in a trivial manner, calculating the distance of each pixel to
every anchor, although a more efficient [algorithm](https://en.wikipedia.org/wiki/Fortune%27s_algorithm) exists.
An even more efficient way could to be to calculate using a [shader](https://nickmcd.me/2020/08/01/gpu-accelerated-voronoi/).
If the `-f, --frames` options is specified a GIF file will be created where
between each frame the anchors take a step in a random direction.

## Usage

The only required option is the `-o, --output_file` when invoking the program.
For the other unspecified arguments various default values be used.
Example, `voronoi --output_file diagram.png` will create a `250x250` PNG file
named `diagram.png` with `10` anchors at random positions with random colors
chosen from a set of `60` random colors.

## Options

The program takes various options that control the creation of the diagram.
+ `-o, --output_file <PATH>` specifies the name of the output file.
+ `-s, --size <NUMBER, ...>` can be used to specify the dimensions of the output file
(PNG/GIF), it can have two forms: `--size 300` uses the same value (`300`) for
the width and the height, while `--size '800, 600'` specifies explicitly the
values for the width (`800`) and the height (`600`). Quotes are mandatory if
the argument contains spaces.
+ `-a, --anchors <NUMBER, ...>` controls the creation of anchors and can have two forms,
first a single number can be specified, `--anchors 30`, this creates `30`
anchors at random coordinates in the image. The second is the form:
`--anchors '300,400; 60,80; 100,100; 200, 0;'` which explicitly specifies the
coordinates (`x, y`) of the anchors . The syntax is: coordinate pairs must be delimited
with a semicolon (`;`) or a newline (`\n`), and each coordinate must be
delimited with a comma (`,`) or whitespace. All other whitespace is ignored.
Quotes are mandatory if the argument contains spaces.
+ `-A, --anchors_from <PATH>` tells the program the read the coordinates of the
anchors from the file specified by `<PATH>`.
The syntax is the same as the `--anchors`.
+ `-c, --colors <NUMBER, ...>` controls the colors that are used to color the
diagram and can have two forms: the first, `--colors 300` tells the program to
choose `300` random colors. The second is the form:
`--colors '38,125,30; 100,200,255; 0,0,0; 255,255,255; 80,180,210;'` which
explicitly specifies the colors (`R, G, B`) to use. The syntax is the same as
the `--anchors` option. Each anchor is in turn associated with a color from
this palette at random. Quotes are mandatory if the argument contains spaces.
+ `-C, --colors_from <PATH>` tells the program to read the colors from the file
specified by `<PATH>`. The syntax is the same as the `--colors` option.
+ `-f, --frames <NUMBER>` tells the program to create a GIF file with `<NUMBER>` frames.
+ `-k, --keep` tells the program to keep the intermediate files when creating a GIF
+ `-s, --seed <NUMBER>` specifies the seed to be used when creating anchors and creating and choosing colors

## Installation

The program can be compiled the following way:

```
git clone https://github.com/begone-prop/voronoi.git
cd voronoi
make
```

## Dependencies
```
GNU Make
gcc
libpng
ImageMagick
```
