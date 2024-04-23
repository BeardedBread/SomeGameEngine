#ifndef __ACTIONS_H
#define __ACTIONS_H
typedef enum ActionType
{
    ACTION_UP=0,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_JUMP,
    ACTION_NEXT_SPAWN,
    ACTION_PREV_SPAWN,
    ACTION_METAL_TOGGLE,
    ACTION_CRATE_ACTIVATION,
    ACTION_CONFIRM,
    ACTION_EXIT,
    ACTION_RESTART,
    ACTION_NEXTLEVEL,
    ACTION_PREVLEVEL,
    ACTION_TOGGLE_GRID,
    ACTION_SET_SPAWNPOINT,
    ACTION_TOGGLE_TIMESLOW,
}ActionType_t;
#endif // __ACTIONS_H
