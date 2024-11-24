// Constants to be used in game
#ifndef __CONSTANTS_H
#define __CONSTANTS_H
#ifndef TILE16_SIZE
#define TILE_SIZE 32
#define DEFAULT_MAP_WIDTH 48
#define DEFAULT_MAP_HEIGHT 32
#define VIEWABLE_EDITOR_MAP_WIDTH 32
#define VIEWABLE_EDITOR_MAP_HEIGHT 16
#define VIEWABLE_MAP_WIDTH 25
#define VIEWABLE_MAP_HEIGHT 14
#else
#define TILE_SIZE 16
#define DEFAULT_MAP_WIDTH 64
#define DEFAULT_MAP_HEIGHT 32
#define VIEWABLE_MAP_WIDTH 32
#define VIEWABLE_MAP_HEIGHT 16
#endif

#define DELTA_T 0.017
#define GRAV_ACCEL 1500
#define JUMP_SPEED 600
#define MOVE_ACCEL 1300

#ifndef TILE16_SIZE
#define PLAYER_WIDTH 22
#define PLAYER_HEIGHT 42
#define PLAYER_C_WIDTH 30
#define PLAYER_C_HEIGHT 26
#else
#define PLAYER_WIDTH 14
#define PLAYER_HEIGHT 30
#define PLAYER_C_WIDTH 14
#define PLAYER_C_HEIGHT 14
#endif
#define PLAYER_C_YOFFSET (PLAYER_HEIGHT - PLAYER_C_HEIGHT)
#define PLAYER_C_XOFFSET (PLAYER_WIDTH - PLAYER_C_WIDTH)

#define PLAYER_MAX_SPEED 800
#define WATER_FRICTION 7.5
#define GROUND_X_FRICTION 6.1
#define GROUND_Y_FRICTION 1.0

#define ARROW_SPEED 400

#define MAX_WATER_LEVEL 4
#define WATER_BBOX_STEP (TILE_SIZE / MAX_WATER_LEVEL)

#define MAX_LEVEL_NUM 50
#endif // __CONSTANTS_H
