import sys
import argparse
import pprint
import json
import struct

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

print("Parsing", args.filename)

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
}

#ENTID_MAPPING = {
#    'Player': 1
#}

# First go to tilesets and find Simple_tiles identifier, then find enumTags to identifier which tile type is what tileid
ids_tiletype_map = {}
tileset_defs = level_pack_data["defs"]["tilesets"]

for ts_def in tileset_defs:
    if ts_def["identifier"] != "Items_spritesheet":
        continue
    for tag in ts_def["enumTags"]:
        ids_tiletype_map[tag["tileIds"][0]] =  ENUMIDS_TILETYPE_MAPPING[tag["enumValueId"]]

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

# Each level should be packed as: [width, 2 bytes][height, 2 bytes][tile_type,entity,water,padding 1,1,1,1 bytes][tile_type,entity,water,padding 1,1,1,1 bytes]...
with open(converted_filename, 'wb+') as out_file:
    out_file.write(struct.pack("<I", n_levels))
    # Then loop the levels. Read the layerIndstances
    for level in all_levels:
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
        # Create a W x H array of tile information
        n_tiles = width * height
        tiles_info = [[0,0,0] for _ in range(n_tiles)]
        # Loop through gridTiles, get "d" as the index to fill the info
        for i, tile in enumerate(level_layout["gridTiles"]):
            try:
                tiles_info[tile["d"][0]][0] = ids_tiletype_map[tile["t"]]
                if tiles_info[tile["d"][0]][0] == ENUMIDS_TILETYPE_MAPPING ["Chest"]:
                    n_chests += 1
            except Exception as e:
                print("Error on tile", i, i % width, i // height)
                print(e)
                tiles_info[tile["d"][0]][0] = 0

        for i, water_level in enumerate(water_layout["intGridCsv"]):
            tiles_info[i][2] = water_level

        # Subject to change
        for ent in entity_layout["entityInstances"]:
            x,y = ent["__grid"]
            tiles_info[y*width + x][0] = ENUMIDS_TILETYPE_MAPPING[ent["__identifier"]]

        out_file.write(struct.pack("<32s4H", level_name.encode('utf-8'), width, height, n_chests, level_tileset))
        for tile in tiles_info:
            out_file.write(struct.pack("<3Bx", *tile))


        #for y in range(height):
        #    for x in range(width):
        #        print(tiles_info[y*width + x], end=" ")
        #    print()

