#!/bin/bash

set -eo pipefail

export LC_ALL=C

grep -aoi '[0-9]' /dev/urandom | tr -d '\n' |
    fold -w 6 | head -n 25 |
    sed -r 's/(...)(...)/\1,\2;/g' #| paste -sd ''
