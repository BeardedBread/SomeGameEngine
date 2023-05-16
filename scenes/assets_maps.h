#ifndef __ASSETS_MAPS_H
#define __ASSETS_MAPS_H

#define N_PLAYER_SPRITES 2
enum PlayerSpriteEnum
{
    SPR_PLAYER_STAND = 0,
    SPR_PLAYER_RUN
};

extern const char* const player_sprite_map[N_PLAYER_SPRITES];

#endif // __ASSETS_MAPS_H
