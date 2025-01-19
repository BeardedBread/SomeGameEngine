import sys
import argparse
import pprint
import json
import struct

from PIL import Image, ImageDraw

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

print("Rendering", args.filename)

with open(args.filename, 'r') as f:
    level_pack_data = json.load(f)

#pprint.pprint(level_pack_data)
ENUMIDS_TILETYPE_MAPPING = {
    'Solid': 1,
    'WoodenPlat': 2,
    'Ladder': 3,
    'LSpike': 4,
    'RSpike': 5,
    'USpike': 6,
    'DSpike': 7,
    'EmptyWCrate': 8,
    'LArrowWCrate': 9,
    'RArrowWCrate': 10,
    'UArrowWCrate': 11,
    'DArrowWCrate': 12,
    'BombWCrate': 13,
    'EmptyMCrate': 14,
    'LArrowMCrate': 15,
    'RArrowMCrate': 16,
    'UArrowMCrate': 17,
    'DArrowMCrate': 18,
    'BombMCrate': 19,
    'Boulder': 20,
    'Runner': 21,
    'Player': 22,
    'Chest': 23,
    'Exit': 24,
    'Urchin': 25,
}

REC_DRAW_FUNCTION = lambda ctx,x,y,s,c : ctx.rectangle(((x,y), (x+s-1, y+s-1)), c) 
UPHALFREC_DRAW_FUNCTION = lambda ctx,x,y,s,c : ctx.rectangle(((x,y), (x+s-1, y+s//2-1)), c) 
DOWNHALFREC_DRAW_FUNCTION = lambda ctx,x,y,s,c : ctx.rectangle(((x, y+s//2), (x+s-1,y+s-1)), c) 
LEFTHALFREC_DRAW_FUNCTION = lambda ctx,x,y,s,c : ctx.rectangle(((x,y), (x+s//2-1, y+s-1)), c) 
RIGHTHALFREC_DRAW_FUNCTION = lambda ctx,x,y,s,c : ctx.rectangle(((x+s//2,y), (x+s-1, y+s-1)), c) 
CIRCLE_DRAW_FUNCTION = lambda ctx,x,y,s,c : ctx.circle((x+s//2, y+s//2), s//2, c) 

TILETYPE_SHAPE_MAP = {
    'Solid': (REC_DRAW_FUNCTION, (0,0,0,255)),
    'WoodenPlat': (UPHALFREC_DRAW_FUNCTION, (128,64,0,255)),
    'Ladder': (REC_DRAW_FUNCTION, (214,141,64,255)),
    'LSpike': (LEFTHALFREC_DRAW_FUNCTION, (239,79,81,255)),
    'RSpike': (RIGHTHALFREC_DRAW_FUNCTION, (239,79,81,255)),
    'USpike': (UPHALFREC_DRAW_FUNCTION, (239,79,81,255)),
    'DSpike': (DOWNHALFREC_DRAW_FUNCTION, (239,79,81,255)),
    'EmptyWCrate': (REC_DRAW_FUNCTION, (160,117,48,255)),
    #'LArrowWCrate': 9,
    #'RArrowWCrate': 10,
    #'UArrowWCrate': 11,
    #'DArrowWCrate': 12,
    #'BombWCrate': 13,
    'EmptyMCrate': (REC_DRAW_FUNCTION, (225,225,225,255)),
    #'LArrowMCrate': 15,
    #'RArrowMCrate': 16,
    #'UArrowMCrate': 17,
    #'DArrowMCrate': 18,
    #'BombMCrate': 19,
    'Boulder': (CIRCLE_DRAW_FUNCTION, (45,45,45,255)),
    #'Runner': 21,
    'Player': (REC_DRAW_FUNCTION, (255,0,255,255)),
    'Chest': (REC_DRAW_FUNCTION, (255,255,0,255)),
    'Exit': (REC_DRAW_FUNCTION, (0,255,0,255)),
    #'Urchin': 25,
}

# First go to tilesets and find Simple_tiles identifier, then find enumTags to identifier which tile type is what tileid
ids_tiletype_map = {}
tileset_defs = level_pack_data["defs"]["tilesets"]

for ts_def in tileset_defs:
    if ts_def["identifier"] != "Items_spritesheet":
        continue
    for tag in ts_def["enumTags"]:
        ids_tiletype_map[tag["tileIds"][0]] =  tag["enumValueId"]

if not ids_tiletype_map:
    print("No tileset definition")
    sys.exit(1)

pprint.pprint(ids_tiletype_map)

def get_level_order(lvl) -> int:
    order = 65535;
    for data in lvl['fieldInstances']:
        if data["__identifier"] == "Order":
            order = data["__value"]
    return order

all_levels = level_pack_data["levels"]
all_levels.sort(key=get_level_order)

# Number of levels is the length of the levels
n_levels = len(all_levels)
print("Number of levels:", n_levels)

fileparts = args.filename.split('.')
if len(fileparts) == 1:
    fileparts.append("lvldat")
else:
    fileparts[-1] = "lvldata"
converted_filename = '.'.join(fileparts)

# First run the entire level pack to figure out the largest dimensions required.

# tile size needs to be dynamic. Fix the output dimensions (and therefore aspect ratio)
# figure out width and height of level
# Get the tile size that fit within the dimensions
# Error out if tile size is zero
# Figure out the offset to center the render
TILE_SIZE = 8
# Each level should be packed as: [width, 2 bytes][height, 2 bytes][tile_type,entity,water,padding 1,1,1,1 bytes][tile_type,entity,water,padding 1,1,1,1 bytes]...
# Then loop the levels. Read the layerIndstances
for level in all_levels[17:18]:
    n_chests : int = 0
    # Search for __identifier for the level layout
    level_name = "" 

    level_metadata = level['fieldInstances']
    level_tileset = 0;
    for data in level_metadata:
        if data["__identifier"] == "TileSet":
            level_tileset = data["__value"]
        if data["__identifier"] == "Name":
            level_name = data["__value"]

    print("Parsing level", level_name)

    level_layout = {}
    entity_layout = {}
    water_layout = {}
    for layer in level['layerInstances']:
        if layer["__identifier"] == "Tiles":
            level_layout = layer
        elif layer["__identifier"] == "Entities":
            entity_layout = layer
        elif layer["__identifier"] == "Water":
            water_layout = layer

    # Dimensions of each level is obtained via __cWid and __cHei. Get the __gridSize as well
    width = level_layout["__cWid"]
    height = level_layout["__cHei"]
    print(f"Dim.: {width}x{height}. N Tiles: {width * height}")
    IMG_WIDTH = width * TILE_SIZE
    IMG_HEIGHT = height * TILE_SIZE
    # Create a W x H array of tile information
    n_tiles = width * height

    lvl_render = Image.new("RGBA", (IMG_WIDTH, IMG_HEIGHT), (255,255,255,0))
    render_ctx = ImageDraw.Draw(lvl_render)
    # Loop through gridTiles, get "d" as the index to fill the info
    for i, tile in enumerate(level_layout["gridTiles"]):
        x, y= (tile["d"][0] % width * TILE_SIZE, tile["d"][0] // width * TILE_SIZE)
        try:
            if ids_tiletype_map[tile["t"]] in TILETYPE_SHAPE_MAP:
                draw_func, colour = TILETYPE_SHAPE_MAP[ids_tiletype_map[tile["t"]]]
                draw_func(render_ctx, x, y, TILE_SIZE, colour)
            else:
                print(ids_tiletype_map[tile["t"]], "is not mapped");
        except Exception as e:
            print("Error on tile", x, y)
            render_ctx.circle((x,y), 2,(255,0,0,255))
            print(e)
    lvl_render.show()
    #for i, water_level in enumerate(water_layout["intGridCsv"]):
    #    tiles_info[i][2] = water_level

    ## Subject to change
    #for ent in entity_layout["entityInstances"]:
    #    x,y = ent["__grid"]
    #    tiles_info[y*width + x][0] = ENUMIDS_TILETYPE_MAPPING[ent["__identifier"]]
    #    if ent["__identifier"] == "Urchin":
    #        spd_encoding = 0
    #        for urchin_data in ent['fieldInstances']:
    #            if urchin_data["__identifier"] == "Direction":
    #                spd_encoding |= urchin_data["__value"] << 2
    #            elif urchin_data["__identifier"] == "SpeedLevel":
    #                spd_encoding |= urchin_data["__value"]

    #        tiles_info[y*width + x][0] += spd_encoding

    
    #for y in range(height):
    #    for x in range(width):
    #        print(tiles_info[y*width + x], end=" ")
    #    print()

