#!/bin/bash
source venv/bin/activate
python ldtk_repacker.py $1.ldtk && zstd $1.lvldata && python level_render.py $1.ldtk
