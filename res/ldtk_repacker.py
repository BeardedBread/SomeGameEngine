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
    'Water': 2
}

ENTID_MAPPING = {
    'Player': 1
}

# First go to tilesets and find Simple_tiles identifier, then find enumTags to identifier which tile type is what tileid
ids_tiletype_map = {}
tileset_defs = level_pack_data["defs"]["tilesets"]

for ts_def in tileset_defs:
    if ts_def["identifier"] != "Simple_tiles":
        continue
    for tag in ts_def["enumTags"]:
        ids_tiletype_map[tag["tileIds"][0]] =  ENUMIDS_TILETYPE_MAPPING[tag["enumValueId"]]

if not ids_tiletype_map:
    print("No tileset definition")
    sys.exit(1)

pprint.pprint(ids_tiletype_map)

# Number of levels is the length of the levels
n_levels = len(level_pack_data["levels"])
print("Number of levels:", n_levels)

fileparts = args.filename.split('.')
if len(fileparts) == 1:
    fileparts.append("lvldat")
else:
    fileparts[-1] = "lvldata"
converted_filename = '.'.join(fileparts)

# Each level should be packed as: [width, 2 bytes][height, 2 bytes][tile_type,entity,water,padding 1,1,1,1 bytes][tile_type,entity,water,padding 1,1,1,1 bytes]...
with open(converted_filename, 'wb+') as out_file:
    out_file.write(struct.pack("I", n_levels))
    # Then loop the levels. Read the layerIndstances
    for level in level_pack_data["levels"]:
        # Search for __identifier for the level layout
        level_name = level["identifier"]
        print("Parsing level", level_name)


        level_layout = {}
        entity_layout = {}
        for layer in level['layerInstances']:
            if layer["__identifier"] == "Tiles":
                level_layout = layer
            elif layer["__identifier"] == "Entities":
                entity_layout = layer

        # Dimensions of each level is obtained via __cWid and __cHei. Get the __gridSize as well
        width = level_layout["__cWid"]
        height = level_layout["__cHei"]
        print(f"Dim.: {width}x{height}")
        # Create a W x H array of tile information
        n_tiles = width * height
        tiles_info = [[0,0,0] for _ in range(n_tiles)]
        # Loop through gridTiles, get "d" as the index to fill the info
        for tile in level_layout["gridTiles"]:
            tiles_info[tile["d"][0]][0] = ids_tiletype_map[tile["t"]]

        for ent in entity_layout["entityInstances"]:
            if ent["__identifier"] in ENTID_MAPPING:
                x,y = ent["__grid"]
                tiles_info[y*width + x][1] = ENTID_MAPPING[ent["__identifier"]]

        out_file.write(struct.pack("32s2H", level_name.encode('utf-8'), width, height))
        for tile in tiles_info:
            out_file.write(struct.pack("3Bx", *tile))


        for y in range(height):
            for x in range(width):
                print(tiles_info[y*width + x], end=" ")
            print()
