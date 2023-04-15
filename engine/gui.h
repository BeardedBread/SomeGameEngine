#ifndef __GUI_H
#define __GUI_H
#include "raylib.h"
#include "raygui.h"

typedef struct UIComp {
    Rectangle bbox;
    GuiState state;
    float alpha;
    bool pressed;
} UIComp_t;

void init_UI(void);
void UI_button(const UIComp_t* bbox, const char* text);
#endif
