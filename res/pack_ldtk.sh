#!/bin/sh

python ldtk_repacker.py $1.ldtk && zstd $1.lvldata
